#ifndef DATA_H
#define DATA_H

typedef struct {
    const char *title;       /* 项目标题 */
    const char *desc;        /* 简介，用 \n 手动换行（最多两行） */
    const char *highlight;   /* 一行核心亮点/特性 */
    const char *tech;        /* 技术栈标签 */
    const char *url;         /* GitHub 仓库链接（"View Source" 按钮） */
    const char *actionLabel; /* 第二个按钮的文字 */
    const char *actionUrl;   /* 第二个按钮的链接；国际象棋项目为 NULL
                                 （isChess 为真时改为进入内嵌试玩，不跳链接） */
    int isChess;             /* 1 表示这是可直接试玩的国际象棋项目 */
} Project;

typedef struct {
    const char *category;
    const char *items;   /* 用 \n 手动换行 */
} SkillGroup;

extern const Project PROJECTS[];
extern const int PROJECT_COUNT;

extern const SkillGroup SKILL_GROUPS[];
extern const int SKILL_GROUP_COUNT;

#define CONTACT_EMAIL "sutharsontuty@gmail.com"
#define CONTACT_GITHUB "https://github.com/sutharson-k"

#endif
