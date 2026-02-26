/*
Copyright 2025 kozakemi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "wifi_impl.h"
#include "../wifi_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief 解码UTF-8转义序列
 */
static int decode_utf8_escape_sequence(const char *input, char *output, size_t output_size)
{
    if (!input || !output || output_size == 0)
    {
        return -1;
    }

    const char *src = input;
    char *dst = output;
    size_t dst_len = 0;

    while (*src && dst_len < output_size - 1)
    {
        if (*src == '\\' && *(src + 1) == 'x' && src + 3 < input + strlen(input))
        {
            char hex_str[3] = {src[2], src[3], '\0'};
            char *endptr;
            unsigned long hex_val = strtoul(hex_str, &endptr, 16);

            if (*endptr == '\0' && hex_val <= 0xFF)
            {
                *dst++ = (char)hex_val;
                dst_len++;
                src += 4;
            }
            else
            {
                *dst++ = *src++;
                dst_len++;
            }
        }
        else
        {
            *dst++ = *src++;
            dst_len++;
        }
    }

    *dst = '\0';
    return 0;
}

wifi_error_t wifi_impl_enable(bool is_enable)
{
    char command[256];
    int result;

    if (is_enable)
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s enable_network all", WIFI_DEVICE);
        result = system(command);
        if (result != 0)
        {
            return WIFI_ERR_TOOL_ERROR;
        }

        snprintf(command, sizeof(command), "wpa_cli -i %s reconnect", WIFI_DEVICE);
        result = system(command);
        if (result != 0)
        {
            return WIFI_ERR_TOOL_ERROR;
        }
    }
    else
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s disable_network all", WIFI_DEVICE);
        system(command);

        snprintf(command, sizeof(command), "wpa_cli -i %s disconnect", WIFI_DEVICE);
        system(command);
    }

    printf("wifi_impl_enable: Wi-Fi %s\n", is_enable ? "enabled" : "disabled");
    return WIFI_ERR_OK;
}

wifi_error_t wifi_impl_scan(bool rescan, wifi_scan_result *result)
{
    FILE *fp;
    char buffer[512];
    char command[256];

    result->networks = NULL;
    result->network_count = 0;

    if (rescan)
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s scan 2>/dev/null", WIFI_DEVICE);
        system(command);
        snprintf(command, sizeof(command), "wpa_cli -i %s scan_results 2>/dev/null", WIFI_DEVICE);
    }
    else
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s scan_results 2>/dev/null", WIFI_DEVICE);
    }

    fp = popen(command, "r");
    if (fp == NULL)
    {
        return WIFI_ERR_TOOL_ERROR;
    }

    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        pclose(fp);
        return WIFI_ERR_TOOL_ERROR;
    }

    wifi_network_info *temp_networks = malloc(sizeof(wifi_network_info) * 64);
    if (!temp_networks)
    {
        pclose(fp);
        return WIFI_ERR_INTERNAL;
    }

    size_t count = 0;
    size_t max_networks = 64;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (count >= max_networks)
        {
            max_networks *= 2;
            wifi_network_info *new_networks =
                realloc(temp_networks, sizeof(wifi_network_info) * max_networks);
            if (!new_networks)
            {
                for (size_t i = 0; i < count; i++)
                {
                    free(temp_networks[i].ssid);
                    free(temp_networks[i].bssid);
                    free(temp_networks[i].security);
                }
                free(temp_networks);
                pclose(fp);
                return WIFI_ERR_INTERNAL;
            }
            temp_networks = new_networks;
        }

        int frequency, signal;
        char bssid[32], security[128], ssid[128];

        char *token = strtok(buffer, "\t");
        if (token)
        {
            strncpy(bssid, token, sizeof(bssid) - 1);
            bssid[sizeof(bssid) - 1] = '\0';
            token = strtok(NULL, "\t");
            if (token)
            {
                frequency = atoi(token);
                token = strtok(NULL, "\t");
                if (token)
                {
                    signal = atoi(token);
                    token = strtok(NULL, "\t");
                    if (token)
                    {
                        char *security_start = token;
                        snprintf(security, sizeof(security), "%s", security_start);

                        token = strtok(NULL, "\t");
                        if (token)
                        {
                            token[strcspn(token, "\n")] = 0;

                            char decoded_ssid[128];
                            if (decode_utf8_escape_sequence(token, decoded_ssid,
                                                            sizeof(decoded_ssid)) == 0)
                            {
                                snprintf(ssid, sizeof(ssid), "%s", decoded_ssid);
                            }
                            else
                            {
                                snprintf(ssid, sizeof(ssid), "%s", token);
                            }
                        }
                        else
                        {
                            ssid[0] = '\0';
                        }

                        temp_networks[count].frequency_mhz = frequency;
                        temp_networks[count].bssid = strdup(bssid);
                        temp_networks[count].signal = signal;
                        temp_networks[count].ssid = strdup(ssid[0] ? ssid : "\\x00");

                        if (strlen(security) == 0)
                        {
                            temp_networks[count].security = strdup("Open");
                        }
                        else
                        {
                            temp_networks[count].security = strdup(security);
                        }

                        if (frequency >= 2412 && frequency <= 2484)
                        {
                            temp_networks[count].channel = (frequency - 2407) / 5;
                        }
                        else if (frequency >= 5035 && frequency <= 5895)
                        {
                            temp_networks[count].channel = (frequency - 5000) / 5;
                        }
                        else
                        {
                            temp_networks[count].channel = 0;
                        }

                        temp_networks[count].recorded = false;
                        count++;
                    }
                }
            }
        }
    }

    pclose(fp);

    snprintf(command, sizeof(command), "wpa_cli -i %s list_networks 2>/dev/null", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL)
    {
        fgets(buffer, sizeof(buffer), fp);

        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            char *token = strtok(buffer, "\t");
            if (token)
            {
                token = strtok(NULL, "\t");
                if (token)
                {
                    token[strcspn(token, "\n")] = 0;

                    char decoded_recorded_ssid[128];
                    if (decode_utf8_escape_sequence(token, decoded_recorded_ssid,
                                                    sizeof(decoded_recorded_ssid)) == 0)
                    {
                        for (size_t i = 0; i < count; i++)
                        {
                            if (strcmp(temp_networks[i].ssid, decoded_recorded_ssid) == 0)
                            {
                                temp_networks[i].recorded = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; i++)
                        {
                            if (strcmp(temp_networks[i].ssid, token) == 0)
                            {
                                temp_networks[i].recorded = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        pclose(fp);
    }

    if (count > 0)
    {
        wifi_network_info *final_networks =
            realloc(temp_networks, sizeof(wifi_network_info) * count);
        if (final_networks)
        {
            temp_networks = final_networks;
        }
    }
    else
    {
        free(temp_networks);
        temp_networks = NULL;
    }

    result->networks = temp_networks;
    result->network_count = count;

    return WIFI_ERR_OK;
}

void wifi_impl_scan_result_free(wifi_scan_result *result)
{
    if (!result || !result->networks)
    {
        return;
    }

    for (size_t i = 0; i < result->network_count; i++)
    {
        free(result->networks[i].ssid);
        free(result->networks[i].bssid);
        free(result->networks[i].security);
    }
    free(result->networks);
    result->networks = NULL;
    result->network_count = 0;
}

wifi_error_t wifi_impl_get_status(wifi_status_info *status)
{
    status->enable = false;
    status->connected = false;
    status->ssid = NULL;
    status->bssid = NULL;
    status->interface = WIFI_DEVICE;
    status->ip = NULL;
    status->signal = 0;
    status->security = NULL;
    status->channel = 0;
    status->frequency_mhz = 0;

    FILE *fp;
    char buffer[256];
    char command[256];

    snprintf(command, sizeof(command),
             "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            status->enable =
                (strstr(buffer, "COMPLETED") != NULL || strstr(buffer, "ASSOCIATED") != NULL ||
                 strstr(buffer, "ASSOCIATING") != NULL || strstr(buffer, "SCANNING") != NULL);
            status->connected = (strstr(buffer, "COMPLETED") != NULL);
        }
        pclose(fp);
    }

    if (status->enable)
    {
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep '^ssid=' | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = 0;
                status->ssid = strdup(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep bssid | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = 0;
                status->bssid = strdup(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "wpa_cli -i %s signal_poll 2>/dev/null | grep RSSI | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                status->signal = atoi(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "ip addr show %s 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1",
                 WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = 0;
                status->ip = strdup(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep key_mgmt | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = 0;
                status->security = strdup(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep channel | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                status->channel = atoi(buffer);
            }
            pclose(fp);
        }

        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep freq | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                status->frequency_mhz = atoi(buffer);
            }
            pclose(fp);
        }
    }

    return WIFI_ERR_OK;
}

void wifi_impl_status_free(wifi_status_info *status)
{
    if (!status)
    {
        return;
    }

    free(status->ssid);
    free(status->bssid);
    free(status->ip);
    free(status->security);

    status->ssid = NULL;
    status->bssid = NULL;
    status->ip = NULL;
    status->security = NULL;
}

wifi_error_t wifi_impl_connect(const char *ssid, const char *password, int timeout_ms)
{
    FILE *fp;
    char buffer[256];
    char command[256];
    int network_id = -1;
    int timeout;
    int wait_time;
    bool connected = false;

    if (!ssid)
    {
        return WIFI_ERR_BAD_REQUEST;
    }

    if (!password || strlen(password) == 0)
    {
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s list_networks 2>/dev/null | grep -E '\t%s\t' | cut -f1",
                 WIFI_DEVICE, ssid);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                network_id = atoi(buffer);
            }
            pclose(fp);
        }

        if (network_id >= 0)
        {
            snprintf(command, sizeof(command), "wpa_cli -i %s enable_network %d 2>/dev/null",
                     WIFI_DEVICE, network_id);
            system(command);

            snprintf(command, sizeof(command), "wpa_cli -i %s select_network %d 2>/dev/null",
                     WIFI_DEVICE, network_id);
            system(command);

            goto wait_for_connection;
        }
    }

    snprintf(command, sizeof(command), "wpa_cli -i %s add_network 2>/dev/null", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            network_id = atoi(buffer);
        }
        pclose(fp);
    }

    if (network_id < 0)
    {
        return WIFI_ERR_TOOL_ERROR;
    }

    snprintf(command, sizeof(command), "wpa_cli -i %s set_network %d ssid '\"%s\"' 2>/dev/null",
             WIFI_DEVICE, network_id, ssid);
    system(command);

    if (password && strlen(password) > 0)
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s set_network %d psk '\"%s\"' 2>/dev/null",
                 WIFI_DEVICE, network_id, password);
    }
    else
    {
        snprintf(command, sizeof(command), "wpa_cli -i %s set_network %d key_mgmt NONE 2>/dev/null",
                 WIFI_DEVICE, network_id);
    }
    system(command);

    snprintf(command, sizeof(command), "wpa_cli -i %s enable_network %d 2>/dev/null", WIFI_DEVICE,
             network_id);
    system(command);

    snprintf(command, sizeof(command), "wpa_cli -i %s select_network %d 2>/dev/null", WIFI_DEVICE,
             network_id);
    system(command);

wait_for_connection:
    timeout = timeout_ms > 0 ? timeout_ms : 20000;
    wait_time = 0;
    connected = false;

    while (wait_time < timeout)
    {
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                if (strstr(buffer, "COMPLETED") != NULL)
                {
                    connected = true;
                    pclose(fp);
                    break;
                }
            }
            pclose(fp);
        }

        sleep(1);
        wait_time += 1000;
    }

    if (!connected)
    {
        if (network_id >= 0)
        {
            snprintf(command, sizeof(command), "wpa_cli -i %s disable_network %d 2>/dev/null",
                     WIFI_DEVICE, network_id);
            system(command);
        }
        return WIFI_ERR_TIMEOUT;
    }

    snprintf(command, sizeof(command), "wpa_cli -i %s save_config 2>/dev/null", WIFI_DEVICE);
    system(command);

    return WIFI_ERR_OK;
}

wifi_error_t wifi_impl_disconnect(const char *ssid)
{
    FILE *fp;
    char buffer[256];
    char command[256];
    bool is_connected = false;

    snprintf(command, sizeof(command),
             "wpa_cli -i %s status 2>/dev/null | grep wpa_state | cut -d= -f2", WIFI_DEVICE);
    fp = popen(command, "r");
    if (fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            if (strstr(buffer, "COMPLETED") != NULL)
            {
                is_connected = true;
            }
        }
        pclose(fp);
    }

    if (!is_connected)
    {
        return WIFI_ERR_NOT_CONNECTED;
    }

    if (ssid && strlen(ssid) > 0)
    {
        char current_ssid[256] = {0};
        snprintf(command, sizeof(command),
                 "wpa_cli -i %s status 2>/dev/null | grep '^ssid=' | head -1 | cut -d= -f2",
                 WIFI_DEVICE);
        fp = popen(command, "r");
        if (fp != NULL)
        {
            if (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = 0;
                snprintf(current_ssid, sizeof(current_ssid), "%s", buffer);
            }
            pclose(fp);
        }

        if (strcmp(current_ssid, ssid) != 0)
        {
            return WIFI_ERR_BAD_REQUEST;
        }
    }

    snprintf(command, sizeof(command), "wpa_cli -i %s disconnect 2>/dev/null", WIFI_DEVICE);
    int result = system(command);

    if (result != 0)
    {
        return WIFI_ERR_TOOL_ERROR;
    }

    return WIFI_ERR_OK;
}