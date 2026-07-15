#ifndef SOUND_H
#define SOUND_H

typedef enum {
    SOUND_MOVE = 0,
    SOUND_CAPTURE = 1,
    SOUND_CHECK = 2,
    SOUND_CASTLE = 3,
    SOUND_GAMEOVER = 4
} SoundType;

/* 必须在用户手势（如点击"开始对局"）内调用，满足浏览器自动播放策略 */
void sound_init(void);
void sound_play(SoundType type);

#endif
