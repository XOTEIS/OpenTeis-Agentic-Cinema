
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <time.h>

    // Definerer tillatte tilstander i systemets ontologi / verktøyflyt
    typedef enum {
        SYS_STATE_IDLE = 0,
        SYS_STATE_AUTHENTICATED,
        SYS_STATE_ACTIVE_EXECUTION,
        SYS_STATE_LOCKED_DOWN
    } SystemState;

    typedef enum {
        ACTION_AUTH = 10,
        ACTION_EXECUTE_TASK = 20,
        ACTION_MODIFY_CORE = 30,
        ACTION_EMERGENCY_LOCK = 40
    } AgentAction;

    typedef struct {
        bool allowed;
        int next_state;
        char denial_reason[128];
        double evaluation_time_ms;
    } SymbolicEvaluationResult;

    double evaluate_symbolic_transition(int current_state, int requested_action, SymbolicEvaluationResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        out_res->allowed = false;
        out_res->next_state = current_state;
        strcpy(out_res->denial_reason, "UNAUTHORIZED_STATE_TRANSITION");

        switch (current_state) {
            case SYS_STATE_IDLE:
                if (requested_action == ACTION_AUTH) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_AUTHENTICATED;
                    strcpy(out_res->denial_reason, "NONE");
                } else if (requested_action == ACTION_EMERGENCY_LOCK) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_LOCKED_DOWN;
                    strcpy(out_res->denial_reason, "NONE");
                } else {
                    strcpy(out_res->denial_reason, "Krev autentisering før utførelse i IDLE-tilstand");
                }
                break;

            case SYS_STATE_AUTHENTICATED:
                if (requested_action == ACTION_EXECUTE_TASK) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_ACTIVE_EXECUTION;
                    strcpy(out_res->denial_reason, "NONE");
                } else if (requested_action == ACTION_EMERGENCY_LOCK) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_LOCKED_DOWN;
                    strcpy(out_res->denial_reason, "NONE");
                } else {
                    strcpy(out_res->denial_reason, "Ugyldig handling for autentisert økt uten aktiv oppgave");
                }
                break;

            case SYS_STATE_ACTIVE_EXECUTION:
                if (requested_action == ACTION_EXECUTE_TASK) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_ACTIVE_EXECUTION;
                    strcpy(out_res->denial_reason, "NONE");
                } else if (requested_action == ACTION_MODIFY_CORE) {
                    // Strenge regler: Ikke tillat kjernemodifikasjon uten spesiell klarering (simulert avvisning)
                    out_res->allowed = false;
                    strcpy(out_res->denial_reason, "KRITISK SIKKERHETSREGEL: Kjernemodifikasjon krever egen sikkerhetsklarering");
                } else if (requested_action == ACTION_EMERGENCY_LOCK) {
                    out_res->allowed = true;
                    out_res->next_state = SYS_STATE_LOCKED_DOWN;
                    strcpy(out_res->denial_reason, "NONE");
                }
                break;

            case SYS_STATE_LOCKED_DOWN:
                // I nedstengt tilstand er alt blokkert unntatt systemrevisjon
                out_res->allowed = false;
                strcpy(out_res->denial_reason, "SYSTEMET ER LÅST NED: Ingen handlinger tillatt");
                break;

            default:
                strcpy(out_res->denial_reason, "UKJENT TILSTAND");
                break;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        out_res->evaluation_time_ms = ms;

        return ms;
    }
    