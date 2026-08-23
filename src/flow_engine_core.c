
    #include <stdio.h>
    #include <string.h>
    #include <stdbool.h>

    typedef enum {
        STATE_GREETING = 0,
        STATE_MAIN_TASK = 1,
        STATE_CONFIDENTIAL_ZONE = 2,
        STATE_BLOCKED_OFFTOPIC = 3
    } AgentState;

    typedef struct {
        int current_state;
        bool transition_blocked;
        char redirect_action[128];
    } FlowResult;

    void evaluate_dialog_transition_c(int current_state, const char* input_text, FlowResult* out_res) {
        out_res->current_state = current_state;
        out_res->transition_blocked = false;
        strcpy(out_res->redirect_action, "ALLOW");

        if (strstr(input_text, "ignore previous instructions") != NULL ||
            strstr(input_text, "system prompt") != NULL ||
            strstr(input_text, "glem alle instruksjoner") != NULL) {

            out_res->current_state = STATE_BLOCKED_OFFTOPIC;
            out_res->transition_blocked = true;
            strcpy(out_res->redirect_action, "REDIRECT_CANONICAL_GUARDRAIL");
            return;
        }

        if (current_state == STATE_GREETING) {
            out_res->current_state = STATE_MAIN_TASK;
        }
    }
    