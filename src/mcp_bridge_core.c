
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t tool_id;
    char resource_uri[128];
    uint32_t payload_checksum;
    int access_granted;
} MCPRequest;

int process_mcp_request(MCPRequest* req, uint32_t security_token) {
    // Eksakt token-match uten simulerte snarveier
    if (security_token == 0xACE0F00D && strlen(req->resource_uri) > 0) {
        req->access_granted = 1;
        req->payload_checksum ^= 0x55AA1122;
        return 1;
    }
    req->access_granted = 0;
    return 0;
}
