/*
 * ============================================================================
 *  项目：STM32 手表 V1 —— 用 ESP32-C3 SuperMini 提供「真实时间 + 天气」
 *  文件：ESP32C3_WeatherClock.ino
 *  平台：Arduino IDE 2.x  +  esp32 开发板包（Board: ESP32C3 Dev Module）
 *
 *  ESP32-C3 负责：连 WiFi → 取网络时间(NTP) → 请求心知天气 → 通过串口把
 *  「UTC秒, 天气代码, 温度」打包成一行发给 STM32。
 *  STM32 负责：收这一行 → 写 RTC → 在 OLED 上显示日期/时间/天气/温度。
 *
 *  接线（两块板都是 3.3V 电平，可直连）：
 *      ESP32-C3 GPIO4 (TX)  --->  STM32 PA3 (USART2_RX)
 *      ESP32-C3 GPIO5 (RX)  <---  STM32 PA2 (USART2_TX)   // 只做单向可只接 TX->RX
 *      ESP32-C3 GND         ---   STM32 GND               // 必须共地
 *  供电：ESP32-C3 用独立 3.3V(≥500mA) 或 USB 供电，不要直接吃手表 LDO。
 *
 *  串口输出协议（一行，\r\n 结尾）：
 *      $OK,<utc秒>,<天气代码>,<温度>\r\n     例：$OK,1763000000,31,17
 *      $ERR,0,0,0\r\n                        例：取数失败时
 *
 *  你只需要改下面「配置区」的 4 个值即可。
 * ============================================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <time.h>
#include <string.h>

/* ===== 编译期配置：直接连接这个 WiFi（不使用 SoftAP 配网） ===== */
#define WIFI_SSID      "Lumos"            /* 改成你的 2.4G WiFi 名称 */
#define WIFI_PASSWORD  "1213812138"       /* 改成你的 WiFi 密码 */
#define DEF_CITY       "guangzhou"        /* 默认城市拼音 */
#define DEF_KEY        "SV7rWPJJVQwUH2Eav"/* 默认心知 key（可在网页里改） */
#define OTA_HOST       "stm32-watch"

static String      g_city, g_key;
static Preferences prefs;
static WebServer   server(80);
static bool        g_needUpdate = false;
static bool        g_web = false;
static volatile int g_keyPending = 0;   /* 网页虚拟按键：先记下，后台再发串口，避免 HTTP 卡住 */

/* 实时状态（供 /status） */
static uint32_t g_lastEpoch = 0, g_lastOkMs = 0;
static int      g_lastCode = -1, g_lastTemp = 0, g_lastDays = 0;
static uint16_t g_okCount = 0, g_failCount = 0;

/* ============================ 一般不用改 ============================ */
#define STM32_TX_PIN  4          // ESP32-C3 的 TX，接 STM32 的 RX(PA3)
#define STM32_RX_PIN  3          // ESP32-C3 的 RX，接 STM32 的 TX(PA2)
#define UART_BAUD     115200
#define UPDATE_INTERVAL_MS  (10UL * 60UL * 1000UL)   // 正常 10 分钟刷新一次

HardwareSerial STM32Serial(1);   // 用 ESP32-C3 的 UART1 和 STM32 通信

/* ============================ 工具函数 ============================ */

/* 连接 WiFi（带 20s 超时），已连接则直接返回 */
static void wifiConnect()
{
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);   // 和诊断程序保持一致：降发射功率，避免认证帧失真
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("正在连接 WiFi: %s ", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi 已连接，IP = ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi 连接失败（检查名称/密码/2.4G 频段）");
  }
}

/* 用 NTP 取时间；成功返回 true，并把 UTC 秒放进 epoch */
static bool syncNtp(uint32_t &epoch)
{
  configTime(0, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "pool.ntp.org");
  time_t now = time(nullptr);
  uint32_t t0 = millis();
  while (now < 1600000000 && millis() - t0 < 15000UL)   // 等到拿到一个合理时间
  {
    delay(300);
    now = time(nullptr);
  }
  if (now < 1600000000) return false;
  epoch = (uint32_t)now;   // time() 永远是 UTC 秒
  return true;
}

/* 备用时间源：拼多多服务器时间接口（NTP 被墙/超时时用） */
static bool httpTime(uint32_t &epoch)
{
  HTTPClient http;
  if (!http.begin("http://api.pinduoduo.com/api/server/_stm")) return false;
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }
  String p = http.getString();
  http.end();

  int i = p.indexOf("server_time");
  if (i < 0) return false;
  i = p.indexOf(':', i);
  if (i < 0) return false;
  i++;
  while (i < (int)p.length() && (p[i] < '0' || p[i] > '9')) i++;

  uint64_t ms = 0;
  while (i < (int)p.length() && p[i] >= '0' && p[i] <= '9')
  {
    ms = ms * 10ULL + (uint64_t)(p[i] - '0');
    i++;
  }
  if (ms < 1600000000000ULL) return false;   // 太少说明没取到
  epoch = (uint32_t)(ms / 1000ULL);
  return true;
}

/* 从一段 JSON 文本里读整数：支持 "key":"123" 或 "key":123 */
static int readJsonInt(const String &s, const char *key, bool &ok)
{
  ok = false;
  int k = s.indexOf(key);
  if (k < 0) return 0;
  int i = s.indexOf(':', k);
  if (i < 0) return 0;
  i++;
  while (i < (int)s.length() && (s[i] == ' ' || s[i] == '"')) i++;

  int sign = 1;
  if (i < (int)s.length() && s[i] == '-') { sign = -1; i++; }

  long v = 0;
  bool any = false;
  while (i < (int)s.length() && s[i] >= '0' && s[i] <= '9')
  {
    v = v * 10 + (s[i] - '0');
    i++;
    any = true;
  }
  ok = any;
  return (int)(sign * v);
}

/* 请求心知天气，拿到天气代码和温度 */
static bool fetchWeather(int &wcode, int &temp)
{
  HTTPClient http;
  String url = String("http://api.seniverse.com/v3/weather/now.json?key=")
             + g_key + "&location=" + g_city + "&language=en&unit=c";

  if (!http.begin(url)) { Serial.println("天气: http.begin 失败"); return false; }
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200)
  {
    Serial.printf("天气: HTTP %d\n", code);
    http.end();
    return false;
  }
  String p = http.getString();
  http.end();

  bool ok1, ok2;
  wcode = readJsonInt(p, "\"code\"", ok1);          // 例 "code":"31"
  temp  = readJsonInt(p, "\"temperature\"", ok2);   // 例 "temperature":"17"
  if (!ok1 || !ok2)
  {
    Serial.print("天气解析失败，原文: ");
    Serial.println(p);
    return false;
  }
  return true;
}

/* 请求多天预报（心知 daily 接口）：返回实际天数。免费版通常只回 3 天，付费最多 15 天 */
#define MAX_DAYS 15
static int fetchForecast(int code[], int hi[], int lo[])
{
  HTTPClient http;
  String url = String("http://api.seniverse.com/v3/weather/daily.json?key=")
             + g_key + "&location=" + g_city + "&language=en&unit=c&start=0&days=" + String(MAX_DAYS);
  if (!http.begin(url)) { Serial.println("预报: http.begin 失败"); return 0; }
  http.setTimeout(8000);
  if (http.GET() != 200) { http.end(); return 0; }
  String p = http.getString();
  http.end();

  int di = p.indexOf("\"daily\"");
  if (di < 0) return 0;
  int ai = p.indexOf('[', di);
  if (ai < 0) return 0;

  int idx = ai + 1, got = 0;
  while (got < MAX_DAYS && idx < (int)p.length())
  {
    int b = p.indexOf('{', idx);
    if (b < 0) break;
    int depth = 0, e = -1;
    for (int k = b; k < (int)p.length(); k++)
    {
      if (p[k] == '{') depth++;
      else if (p[k] == '}') { depth--; if (depth == 0) { e = k; break; } }
    }
    if (e < 0) break;
    String obj = p.substring(b, e + 1);
    bool o1, o2, o3;
    int c = readJsonInt(obj, "\"code_day\"", o1);
    int h = readJsonInt(obj, "\"high\"", o2);
    int l = readJsonInt(obj, "\"low\"", o3);
    if (!(o1 && o2 && o3)) break;
    code[got] = c; hi[got] = h; lo[got] = l;
    got++;
    idx = e + 1;
  }
  return got;
}

/* CRC-16/CCITT-FALSE：poly=0x1021, init=0xFFFF, 不反转、不异或输出。
   两端用同一算法；对 "$" 起到 '*' 之前的整段 ASCII 计算，附在 '*' 后 4 位十六进制。 */
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}

/* ===== 可靠传输：序号 + ACK + 超时重传（去重由 STM32 端完成） ===== */
#define ACK_TIMEOUT_MS 200
#define FRAME_RETRY    3
static uint8_t g_seq = 0;

/* 统一组帧：body 形如 "$OK,<seq>,..."（不含 CRC），发送 body*HHHH\r\n */
static void sendFrame(const char *body)
{
  uint16_t c = crc16_ccitt((const uint8_t *)body, (uint16_t)strlen(body));
  STM32Serial.printf("%s*%04X\r\n", body, c);
}

/* 等待 STM32 回 "$AK,<seq>"（超时返回 false） */
static bool waitAck(uint8_t seq, uint32_t timeout_ms)
{
  uint32_t t0 = millis();
  char buf[40];
  int bi = 0;
  while ((uint32_t)(millis() - t0) < timeout_ms)
  {
    while (STM32Serial.available())
    {
      char c = (char)STM32Serial.read();
      if (c == '\n')
      {
        int s = -1;
        buf[bi] = '\0';
        if (sscanf(buf, "$AK,%d", &s) == 1 && (uint8_t)s == seq) return true;
        bi = 0;
      }
      else if (c != '\r')
      {
        if (bi < (int)sizeof(buf) - 1) buf[bi++] = c;
        else bi = 0;
      }
    }
    delay(3);
  }
  return false;
}

/* 可靠发送：同一 seq 最多发 FRAME_RETRY 次，直到收到 ACK */
static bool sendReliable(const char *body)
{
  uint8_t seq = g_seq;
  int attempt;
  for (attempt = 0; attempt < FRAME_RETRY; attempt++)
  {
    sendFrame(body);
    if (waitAck(seq, ACK_TIMEOUT_MS)) { g_seq++; return true; }
    Serial.printf("重传 seq=%u (第 %d 次)\r\n", seq, attempt + 1);
  }
  g_seq++;
  return false;
}

/* 把结果发一行给 STM32（带 seq + CRC + 重传） */
static void sendLine(bool ok, uint32_t epoch, int wcode, int temp)
{
  char buf[64];
  if (ok)
    snprintf(buf, sizeof(buf), "$OK,%u,%lu,%d,%d", (unsigned)g_seq, (unsigned long)epoch, wcode, temp);
  else
    snprintf(buf, sizeof(buf), "$ERR,%u", (unsigned)g_seq);
  sendReliable(buf);
}

/* 一次完整刷新：WiFi → 时间 → 天气 → 发送 */
static void doUpdate()
{
  wifiConnect();

  if (WiFi.status() != WL_CONNECTED)
  {
    g_failCount++;
    sendLine(false, 0, 0, 0);
    return;
  }

  uint32_t epoch = 0;
  bool tok = syncNtp(epoch);
  if (!tok) tok = httpTime(epoch);

  int wcode = 0, temp = 0;
  bool wok = fetchWeather(wcode, temp);

  /* 时间取到就先把时间同步过去；天气失败用 code=-1 表示“本次无天气”，
     STM32 会保留上一次的天气显示，但时间照样校准 */
  if (!tok)
  {
    g_failCount++;
    sendLine(false, 0, 0, 0);
    Serial.println("发送 -> 时间获取失败");
    return;
  }
  if (!wok) { wcode = -1; temp = 0; }

  sendLine(true, epoch, wcode, temp);
  g_lastEpoch = epoch;
  g_lastCode  = wcode;
  g_lastTemp  = temp;
  g_lastOkMs  = millis();
  g_okCount++;
  Serial.printf("发送 -> utc=%lu  code=%d  temp=%d%s\r\n",
                (unsigned long)epoch, wcode, temp, wok ? "" : "  (天气本次失败)");

  /* 多天预报：
     - $FC 前 3 天 → 天气列表页（今日/明日/后日）
     - $FD,<n>,c0,h0,l0,... 全部天数 → 温度趋势曲线页
     免费版 key 实际只回 3 天；付费 key 最多 15 天，代码无需改。 */
  {
    int fcode[MAX_DAYS], fhi[MAX_DAYS], flo[MAX_DAYS];
    int fn = fetchForecast(fcode, fhi, flo);
    if (fn > 0)
    {
      int i;
      char buf[320];
      /* 不足 3 天则用最后一天补齐，保证列表页有 3 行 */
      for (i = fn; i < 3; i++) { fcode[i] = fcode[fn - 1]; fhi[i] = fhi[fn - 1]; flo[i] = flo[fn - 1]; }

      snprintf(buf, sizeof(buf), "$FC,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               (unsigned)g_seq,
               fcode[0], fhi[0], flo[0], fcode[1], fhi[1], flo[1], fcode[2], fhi[2], flo[2]);
      sendReliable(buf);

      /* $FD,<seq>,<n>,c0,h0,l0,... 全部天数（趋势曲线页） */
      {
        int p = snprintf(buf, sizeof(buf), "$FD,%u,%d", (unsigned)g_seq, fn);
        for (i = 0; i < fn && p < (int)sizeof(buf) - 16; i++)
          p += snprintf(buf + p, sizeof(buf) - p, ",%d,%d,%d", fcode[i], fhi[i], flo[i]);
        sendReliable(buf);
      }

      Serial.printf("预报 -> n=%d  %d/%d/%d ...\r\n", fn, fhi[0], flo[0], fcode[0]);
      g_lastDays = fn;
    }
    else
    {
      Serial.println("预报获取失败");
    }
  }
}

/* ============================ 配置页 / 虚拟按键 / OTA ============================ */
static String pageHtml()
{
  String h;
  h += "<!doctype html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>STM32 Watch</title>";
  h += "<style>body{font-family:-apple-system,Arial;margin:18px;background:#111;color:#eee}";
  h += "input{width:100%;padding:10px;margin:6px 0;box-sizing:border-box;border-radius:6px;border:1px solid #333;background:#1b1b1b;color:#eee}";
  h += "button{padding:14px 22px;margin:6px;font-size:18px;border:0;border-radius:10px;background:#2d7ef7;color:#fff}";
  h += ".row{display:flex;justify-content:center;flex-wrap:wrap}</style></head><body>";
  h += "<h2>STM32 手表 · ESP32-C3</h2>";
  h += "<h3>实时状态</h3><pre id='st' style='background:#1b1b1b;padding:10px;border-radius:8px;white-space:pre-wrap;margin:0'>加载中…</pre>";

  h += "<h3>城市 / 天气设置</h3><form method='POST' action='/save'>";
  h += "城市拼音(如 guangzhou)<input name='city' value='" + g_city + "'>";
  h += "心知 key<input name='key' value='" + g_key + "'>";
  h += "<button type='submit'>保存并立即刷新</button></form>";

  h += "<hr><h3>手机虚拟按键</h3><div class='row'>";
  h += "<button onclick=\"k('1')\">▲ 上</button>";
  h += "<button onclick=\"k('3')\">● 确认</button>";
  h += "<button onclick=\"k('2')\">▼ 下</button></div>";
  h += "<p style='color:#888'>点按钮 = 在 STM32 上模拟一次对应按键 · OTA 主机名 " OTA_HOST "</p>";
  h += "<script>function k(v){fetch('/key?k='+v);}";
  h += "function rs(){fetch('/status').then(r=>r.json()).then(d=>{";
  h += "document.getElementById('st').textContent=";
  h += "'IP: '+d.ip+'\\n信号: '+d.rssi+' dBm\\n运行: '+d.uptime+' s\\n城市: '+d.city";
  h += "+'\\n天气码: '+d.code+'  温度: '+d.temp+'C\\n预报天数: '+d.days";
  h += "+'\\n距上次成功: '+d.lastOkAgo+' s\\n成功: '+d.ok+'  失败: '+d.fail;";
  h += "});} rs();setInterval(rs,2000);</script></body></html>";
  return h;
}

static void handleRoot() { server.send(200, "text/html", pageHtml()); }

/* /status：返回 C3 侧实时状态 JSON */
static void handleStatus()
{
  char buf[320];
  uint32_t up  = millis() / 1000UL;
  uint32_t ago = g_lastOkMs ? (millis() - g_lastOkMs) / 1000UL : 0;
  snprintf(buf, sizeof(buf),
           "{\"ip\":\"%s\",\"rssi\":%d,\"uptime\":%lu,\"city\":\"%s\",\"code\":%d,"
           "\"temp\":%d,\"days\":%d,\"lastOkAgo\":%lu,\"ok\":%u,\"fail\":%u}",
           WiFi.localIP().toString().c_str(), WiFi.RSSI(), (unsigned long)up,
           g_city.c_str(), g_lastCode, g_lastTemp, g_lastDays,
           (unsigned long)ago, (unsigned)g_okCount, (unsigned)g_failCount);
  server.send(200, "application/json", buf);
}

static void handleSave()
{
  if (server.hasArg("city")) prefs.putString("city", server.arg("city"));
  if (server.hasArg("key"))  prefs.putString("key",  server.arg("key"));
  g_city = prefs.getString("city", DEF_CITY);
  g_key  = prefs.getString("key",  DEF_KEY);
  g_needUpdate = true;                 /* 让主循环立即刷新一次天气 */
  server.send(200, "text/html", "<meta charset='utf-8'>已保存，正在刷新天气…（可返回）");
}

/* 手机虚拟按键：/key?k=1(上) / 2(下) / 3(确认) → 串口 $KEY 帧
   注意：这里只登记，实际发送放到 loop() 里做，避免 HTTP 等待 ACK 造成延迟 */
static void handleKey()
{
  String k = server.arg("k");
  int code = -1;
  if (k == "1" || k == "up")        code = 1;
  else if (k == "2" || k == "down") code = 2;
  else if (k == "3" || k == "ok")   code = 3;

  if (code < 0) { server.send(400, "text/plain", "bad key"); return; }

  g_keyPending = code;
  server.send(200, "text/plain", "ok");
}

static void startWeb()
{
  if (g_web) return;
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/key", handleKey);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  g_web = true;
}

/* ============================ setup / loop ============================ */
void setup()
{
  Serial.begin(115200);          // USB 调试口
  STM32Serial.begin(UART_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  prefs.begin("watch", false);
  g_city = prefs.getString("city", DEF_CITY);
  g_key  = prefs.getString("key",  DEF_KEY);

  delay(300);
  Serial.println();
  Serial.println("==== ESP32-C3 天气时钟数据源 启动 ====");
  Serial.printf("WiFi=%s  city=%s\n", WIFI_SSID, g_city.c_str());

  wifiConnect();

  if (WiFi.status() == WL_CONNECTED)
  {
    startWeb();
    ArduinoOTA.setHostname(OTA_HOST);
    ArduinoOTA.begin();
    Serial.print("Web / OTA: http://");
    Serial.println(WiFi.localIP());
    doUpdate();                    // 开机立刻发一次
  }
  else
  {
    Serial.println("WiFi 连接失败：检查顶部 WIFI_SSID / WIFI_PASSWORD（之后每 5s 自动重连）");
  }
}

void loop()
{
  /* WiFi 断开时每 5s 重连一次 */
  if (WiFi.status() != WL_CONNECTED)
  {
    static uint32_t lastTry = 0;
    if (millis() - lastTry > 5000UL)
    {
      lastTry = millis();
      wifiConnect();
      if (WiFi.status() == WL_CONNECTED && !g_web)
      {
        startWeb();
        ArduinoOTA.setHostname(OTA_HOST);
        ArduinoOTA.begin();
        Serial.print("Web / OTA: http://");
        Serial.println(WiFi.localIP());
        g_needUpdate = true;
      }
    }
  }

  server.handleClient();
  if (WiFi.status() == WL_CONNECTED) ArduinoOTA.handle();

  /* STM32 可以发 'R' 主动要求刷新一次（PA2 -> GPIO5） */
  while (STM32Serial.available())
  {
    char c = (char)STM32Serial.read();
    if (c == 'R' || c == 'r') doUpdate();
  }

  /* 网页虚拟按键：后台发送（网页已先行返回，不受 ACK 等待影响） */
  if (g_keyPending)
  {
    int code = g_keyPending;
    g_keyPending = 0;
    char kb[32];
    snprintf(kb, sizeof(kb), "$KEY,%u,%d", (unsigned)g_seq, code);
    sendReliable(kb);
  }

  /* 网页保存了城市/key → 立即刷新一次 */
  if (g_needUpdate)
  {
    g_needUpdate = false;
    doUpdate();
  }

  /* 正常定时刷新：开机前 3 分钟每 20s，之后 10 分钟一次 */
  if (WiFi.status() == WL_CONNECTED)
  {
    static uint32_t last = 0;
    static uint32_t bootMs = 0;
    if (bootMs == 0) bootMs = millis();
    uint32_t interval = (millis() - bootMs < 180000UL) ? 20000UL : UPDATE_INTERVAL_MS;
    if (millis() - last >= interval)
    {
      last = millis();
      doUpdate();
    }
  }
  delay(20);
}
