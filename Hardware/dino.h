#ifndef __DINO_H__
#define __DINO_H__

#include <stdint.h>

/*存放图标x,y最小值和最大值的结构体*/
/* 用有符号类型：障碍物移出屏幕左侧时 X 会变成负数，如果用 uint8_t 会回绕 */
struct Object_Position
{
    int16_t minX, minY, maxX, maxY;
};

int DinoGame_Animation(void);
void Dino_Tick(void);
void DinoGame_Pos_Init(void);
int isColliding(struct Object_Position *a, struct Object_Position *b);


#endif /* __DINO_H__ */
