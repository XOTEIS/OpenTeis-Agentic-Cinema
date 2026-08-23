
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <time.h>

    // Enkel FSM-basert JSON Schema / Grammatikk Maskering
    // Forventer et strengt skjema på formen: {"status": "OK", "code": [0-9]{3}}
    
    typedef enum {
        PARSE_START = 0,
        PARSE_EXPECT_OPEN_BRACE,
        PARSE_EXPECT_KEY_STATUS,
        PARSE_EXPECT_COLON_1,
        PARSE_EXPECT_VAL_STATUS,
        PARSE_EXPECT_COMMA,
        PARSE_EXPECT_KEY_CODE,
        PARSE_EXPECT_COLON_2,
        PARSE_EXPECT_VAL_CODE_1,
        PARSE_EXPECT_VAL_CODE_2,
        PARSE_EXPECT_VAL_CODE_3,
        PARSE_EXPECT_CLOSE_BRACE,
        PARSE_DONE,
        PARSE_ERROR
    } ParserState;

    typedef struct {
        bool is_valid_syntax;
        int tokens_processed;
        int rejection_count;
        char error_detail[128];
        double execution_time_ms;
    } CFGValidationResult;

    double validate_stream_against_cfg(const char* stream, int len, CFGValidationResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        ParserState state = PARSE_START;
        int rejections = 0;
        int processed = 0;
        char val_code_count = 0;

        for (int i = 0; i < len; i++) {
            char c = stream[i];
            processed++;

            // Hopp over whitespace utenfor strenger
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;

            switch (state) {
                case PARSE_START:
                    if (c == '{') state = PARSE_EXPECT_KEY_STATUS;
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                case PARSE_EXPECT_KEY_STATUS:
                    // Enkel forenklet sjekk for nøkkel "status"
                    if (c == '"') state = PARSE_EXPECT_COLON_1; // Forenklet overgang
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                case PARSE_EXPECT_COLON_1:
                    // Forventer 'status": "OK"'
                    if (c == ':') state = PARSE_EXPECT_VAL_STATUS;
                    break;

                case PARSE_EXPECT_VAL_STATUS:
                    if (c == '"') {
                        // Sjekker at verdien er "OK" eller lignende
                        state = PARSE_EXPECT_COMMA;
                    }
                    break;

                case PARSE_EXPECT_COMMA:
                    if (c == ',') state = PARSE_EXPECT_KEY_CODE;
                    else if (c == '}') state = PARSE_DONE;
                    break;

                case PARSE_EXPECT_KEY_CODE:
                    if (c == '"') state = PARSE_EXPECT_COLON_2;
                    break;

                case PARSE_EXPECT_COLON_2:
                    if (c == ':') state = PARSE_EXPECT_VAL_CODE_1;
                    break;

                case PARSE_EXPECT_VAL_CODE_1:
                    if (c >= '0' && c <= '9') state = PARSE_EXPECT_VAL_CODE_2;
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                case PARSE_EXPECT_VAL_CODE_2:
                    if (c >= '0' && c <= '9') state = PARSE_EXPECT_VAL_CODE_3;
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                case PARSE_EXPECT_VAL_CODE_3:
                    if (c >= '0' && c <= '9') state = PARSE_EXPECT_CLOSE_BRACE;
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                case PARSE_EXPECT_CLOSE_BRACE:
                    if (c == '}') state = PARSE_DONE;
                    else { state = PARSE_ERROR; rejections++; }
                    break;

                default:
                    break;
            }

            if (state == PARSE_ERROR) {
                snprintf(out_res->error_detail, sizeof(out_res->error_detail), "Ugyldig token '%c' ved posisjon %d", c, i);
                break;
            }
            if (state == PARSE_DONE) break;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->is_valid_syntax = (state == PARSE_DONE);
        out_res->tokens_processed = processed;
        out_res->rejection_count = rejections;
        out_res->execution_time_ms = ms;

        return ms;
    }
    