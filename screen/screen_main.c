#include <stdio.h>
#include "../util/texts.h"
#include "../util/uiutil.h"
#include "../util/debug.h"
#include "../util/mystructures.h"
#include "../util/keylistener.h"
#include "../service.h"
#include "../db.h"
#include "../video.h"
#include "../user.h"
#include "screen.h"

static String* receiveText(int minLen, char *notice, char* warning) {
    StringBuilder *sb = newStringBuilder();
    appendSpaceStringBuilder(sb);
    appendMStringBuilder(sb, 4,SIMPLE_LINE, notice, "\n", SIMPLE_LINE);
    printf(sb->string);
    String *input = readline();
    while(countUtf8Char(input) < minLen) {
        printf("%s\n", warning);
        freeString(input);
        input = readline();
    }
    freeStringBuilder(sb);
    return input;
}

static void receiveBirthday(Date *target) {
    StringBuilder *sb = newStringBuilder();
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendStringBuilder(sb, "태어난 연도를 입력해주세요. ex) 2003\n");
        printf(sb->string);
        scanf("%d", &target->year);
        if(target->year < 1800) {
            dialog("잘못된 년도를 입력하셨습니다.");
        } else break;
    }
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendStringBuilder(sb, "태어난 달을 입력해주세요. ex) 10\n");
        printf(sb->string);
        scanf("%d", &target->month);
        if(target->month < 1 || target->month > 12) {
            dialog("잘못된 값을 입력하셨습니다.");
        } else break;
    }
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendStringBuilder(sb, "태어난 일을 입력해주세요. ex) 5\n");
        printf(sb->string);
        scanf("%d", &target->day);
        if(target->day < 1 || !isValidDate(target)) {
            dialog("잘못된 값을 입력하셨습니다.");
        } else break;
    }
    freeStringBuilder(sb);
}

static void loginScreen(Service *service) {
    String *inputId = receiveText(5, "환영합니다!\n\n아이디를 입력해주세요.", "5자 이상의 값을 입력해주세요.");
    String *inputPassword = receiveText(8, "비밀번호를 입력해주세요.", "8자 이상의 값을 입력해주세요.");
    int response = tryLoginService(service, inputId, inputPassword);
    freeString(inputId);
    freeString(inputPassword);
}

void userEditScreen(Service *service, User *base) {
    SignupForm form;
    form.birthday = base->birthday;
    int selectedField = 0;
    int fields = 5;
    String *strings[4] = {base->name, base->loginId, newString(""), base->phone};
    String *values[4];
    for(int i = 0; i<4; i++) values[i] = cloneString(strings[i]);
    freeString(strings[2]);
    char keys[][24] = {"이름", "아이디", "패스워드", "전화번호"};
    int minLen[] = {2,5,8,5};
    StringBuilder *sb = newStringBuilder();
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 4, SIMPLE_LINE, "회원 정보 수정\n", SIMPLE_LINE, "\n");
        // 이름 ~ 전화번호 Option
        int pwLen = countUtf8Char(values[2]);
        for(int i = 0; i<fields-1; i++) {
            if(selectedField == i) appendStringBuilder(sb, "▶ ");
            appendStringBuilder(sb, keys[i]);
            appendStringBuilder(sb, ": ");
            if(i != 2) appendStringBuilder(sb, values[i]->content);
            else for(int c = 0; c<pwLen; c++) appendCharStringBuilder(sb, '*');
            appendlnStringBuilder(sb);
            appendlnStringBuilder(sb);
        }
        // 생일 Option
        if(selectedField == fields-1) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "생일: ");
        String *birthToString = toStringDate(&form.birthday);
        appendStringBuilder(sb, birthToString->content);
        freeString(birthToString);
        appendStringBuilder(sb, "\n\n");
        // 가입하기 Option
        if(selectedField == fields) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "<수정하기>\n");
        // Tooltip
        appendMStringBuilder(sb, 4, "\n", SIMPLE_LINE, "⬆⬇       선택\nEnter      확인/편집\nESC        나가기\n", SIMPLE_LINE);
        printf(sb->string);
        int keyInput = mygetch();
        if(keyInput == KEYS.arrowDown) {
            selectedField = boundaryMove(selectedField, fields+1, 1);
            continue;
        }
        if(keyInput == KEYS.arrowUp) {
            selectedField = boundaryMove(selectedField, fields+1, -1);
            continue;
        }
        if(keyInput == KEYS.esc) {
            break;
        }
        if(keyInput != KEYS.enter) continue;
        if(selectedField == 4) { // 생일
            receiveBirthday(&form.birthday);
            continue;
        }
        if(selectedField == 5) { // 가입하기
            form.name = values[0];
            form.loginId = values[1];
            form.password = values[2];
            form.phone = values[3];

            Response response = editUserService(service, base->id, form);
            if(response.succeed) {
                freeString(response.msg);
                dialog("회원 정보 수정이 완료되었습니다.");
                break; 
            }
            dialog(response.msg->content);
            freeString(response.msg);
            continue;
        }
        StringBuilder *notice = newStringBuilder();
        appendStringBuilder(notice, keys[selectedField]);
        appendStringBuilder(notice, "을(를) 입력해주세요.");
        StringBuilder *warning = newStringBuilder();
        appendIntStringBuilder(warning, minLen[selectedField]);
        appendStringBuilder(warning, "자 이상 입력해주세요.");
        String *input = receiveText(minLen[selectedField], notice->string, warning->string);
        freeStringBuilder(notice);
        freeStringBuilder(warning);
        freeString(values[selectedField]);
        values[selectedField] = input;
    }
    for(int i = 0; i<4; i++) freeString(values[i]);
    freeStringBuilder(sb);
    d("end of userEditScreen");
    return;

}

static void signupScreen(Service *service) {
    SignupForm form;
    form.birthday = (Date) {2003, 10, 5};
    int selectedField = 0;
    int fields = 5;
    String *values[5]; for(int i=0;i<5;i++)values[i]=newString("");
    char keys[][24] = {"이름", "아이디", "패스워드", "전화번호"};
    int minLen[] = {2,5,8,5};
    StringBuilder *sb = newStringBuilder();
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendMStringBuilder(sb, 4, SIMPLE_LINE, "회원 가입\n", SIMPLE_LINE, "\n");
        // 이름 ~ 전화번호 Option
        int pwLen = countUtf8Char(values[2]);
        for(int i = 0; i<fields-1; i++) {
            if(selectedField == i) appendStringBuilder(sb, "▶ ");
            appendStringBuilder(sb, keys[i]);
            appendStringBuilder(sb, ": ");
            if(i != 2) appendStringBuilder(sb, values[i]->content);
            else for(int c = 0; c<pwLen; c++) appendCharStringBuilder(sb, '*');
            appendlnStringBuilder(sb);
            appendlnStringBuilder(sb);
        }
        // 생일 Option
        if(selectedField == fields-1) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "생일: ");
        String *birthToString = toStringDate(&form.birthday);
        appendStringBuilder(sb, birthToString->content);
        freeString(birthToString);
        appendStringBuilder(sb, "\n\n");
        // 가입하기 Option
        if(selectedField == fields) appendStringBuilder(sb, "▶ ");
        appendStringBuilder(sb, "<가입하기>\n");
        // Tooltip
        appendMStringBuilder(sb, 4, "\n", SIMPLE_LINE, "⬆⬇       선택\nEnter      확인/편집\nESC        나가기\n", SIMPLE_LINE);
        printf(sb->string);
        int keyInput = mygetch();
        if(keyInput == KEYS.arrowDown) {
            selectedField = boundaryMove(selectedField, fields+1, 1);
            continue;
        }
        if(keyInput == KEYS.arrowUp) {
            selectedField = boundaryMove(selectedField, fields+1, -1);
            continue;
        }
        if(keyInput == KEYS.esc) {
            break;
        }
        if(keyInput != KEYS.enter) continue;
        if(selectedField == 4) { // 생일
            receiveBirthday(&form.birthday);
            continue;
        }
        if(selectedField == 5) { // 가입하기
            form.name = values[0];
            form.loginId = values[1];
            form.password = values[2];
            form.phone = values[3];
            Response response = signupService(service, form);
            if(response.succeed) {
                dialog("환영합니다! 회원가입에 성공하셨습니다.");
                break; 
            }
            dialog(response.msg->content);
            freeString(response.msg);
            continue;
        }
        StringBuilder *notice = newStringBuilder();
        appendStringBuilder(notice, keys[selectedField]);
        appendStringBuilder(notice, "을(를) 입력해주세요.");
        StringBuilder *warning = newStringBuilder();
        appendIntStringBuilder(warning, minLen[selectedField]);
        appendStringBuilder(warning, "자 이상 입력해주세요.");
        String *input = receiveText(minLen[selectedField], notice->string, warning->string);
        freeStringBuilder(notice);
        freeStringBuilder(warning);
        freeString(values[selectedField]);
        values[selectedField] = input;
    }
    for(int i = 0; i<fields; i++) freeString(values[i]);
    freeStringBuilder(sb);
    return;
}

void mainScreen(Service *service) {
    StringBuilder *sb = newStringBuilder();
    int selectedOption = 0;
    static char optionNames[][24] = {
        "로그인", "회원가입"
    };
    int optionSize = 2;
    while(1) {
        sb->size = 0;
        appendSpaceStringBuilder(sb);
        appendStringBuilder(sb, SIMPLE_LINE);
        appendlnStringBuilder(sb);
        for(int i = 0; i<optionSize; i++) {
            if(selectedOption == i) appendStringBuilder(sb, "▶");
            appendMStringBuilder(sb, 3, " ", optionNames[i], "\n\n");
        }
        appendStringBuilder(sb, SIMPLE_LINE);
        appendMStringBuilder(sb, 2, TOOLTIP_CHOICE, TOOLTIP_CONFIRM);
        appendStringBuilder(sb, SIMPLE_LINE);
        printf(sb->string);
        int key = mygetch();
        if(key == KEYS.arrowDown) {
            selectedOption = boundaryMove(selectedOption, optionSize, 1);
            continue;
        }
        if(key == KEYS.arrowUp) {
            selectedOption = boundaryMove(selectedOption, optionSize, -1);
            continue;
        }
        if(key != KEYS.enter) continue;
        if(selectedOption == 0) {
            loginScreen(service);
            User *logined = getLoginService(service);
            if(logined != NULL) {
                if(logined->id == 0) adminMainScreen(service);
                else userMainScreen(service);
            }
            else dialog("아이디 또는 패스워드가 잘못되었습니다.");
            continue;
        }
        signupScreen(service);
    }
    freeStringBuilder(sb);
}