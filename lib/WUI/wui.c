/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#include "wui.h"
#include "wui_vars.h"
#include "marlin_client.h"
#include "wui_api.h"
#include "lwip.h"
#include "ethernetif.h"
#include <string.h>
#include "sntp_client.h"
#include "dbg.h"
#include "httpd.h"
#include "mbedtls_config.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"

#define WUI_NETIF_SETUP_DELAY  1000
#define WUI_COMMAND_QUEUE_SIZE WUI_WUI_MQ_CNT // maximal number of messages at once in WUI command messageQ

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name

unsigned char memory_buf[40000];

#define CERT_RSA_2048                                                    \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIIDFzCCAf8CFA8hXppeZutjOjITrou74JwEHX8oMA0GCSqGSIb3DQEBCwUAMEgx\n" \
    "CzAJBgNVBAYTAlNBMRAwDgYDVQQIDAdDemVjaGlhMQ8wDQYDVQQHDAZQcmFndWUx\n" \
    "FjAUBgNVBAoMDVBydXNhIHRlc3QgQ0EwHhcNMjEwMTE1MTUzMzU3WhcNMjEwMjE0\n" \
    "MTUzMzU3WjBIMQswCQYDVQQGEwJTQTEQMA4GA1UECAwHQ3plY2hpYTEPMA0GA1UE\n" \
    "BwwGUHJhZ3VlMRYwFAYDVQQKDA1QcnVzYSB0ZXN0IENBMIIBIjANBgkqhkiG9w0B\n" \
    "AQEFAAOCAQ8AMIIBCgKCAQEAwUKbzdQ4ok4sEC42QKhrzUqsQnZ9oFBdScCeFRut\n" \
    "w1zufUDSk4CvMjn4IF099ecu8/0b2jc8Pf4jjgAeY++kn49b+vkM/Y/q9tbS9MHV\n" \
    "6v8DSpQ8vgvFpKyq+VXojAt48kzFZYqEuH30XsBFspepwy6p0Ut2orZYrFGXs+es\n" \
    "hL6m4ZuV8K6TiNsSNtJGbzUv/Lg2AjNj84WECapvCLTZsFLoygnS+zU/K8U/lrg7\n" \
    "6U8z61OP9bU9DqI2hPzIaR9wLaNUbcVV7DWvOw9Beyh2bbZT1EhqIjxnyVyahQRU\n" \
    "60sAeeYT1TBz+hWXwhOkiOiUn8dNQUfizyzOM1JIcntWCwIDAQABMA0GCSqGSIb3\n" \
    "DQEBCwUAA4IBAQA6ejtManThFJoH+IP+kFhUGFMOLikV4eLKvbMUu4rCBDVS5QVZ\n" \
    "tDCzKR2I5Li9gEOl0UiHcq4XUmy1D1dkJkZT344wGonrVVmnYuBcwsA4V/YnRYUV\n" \
    "2ocw08z8V2ed41HsWZrOXZjnBjXFmVLt5srVFk5DKji5k1hTZkgJojDghC1/0RW1\n" \
    "OuU3hO9DKpLd7AG8s3KGeh0ZhbGnkjA7CZW7+UIUu+/43qBP5DsnpgpYMcVF9Lxk\n" \
    "pPV/F3STOiutN2SSr9ya9OnCG6+VYcci+ALKneUoAyz134pDqqd+co4/kB3zvrEe\n" \
    "g/n8rxhCoHvF++2ekqE8Bg/veerqW/nxAhKr\n"                             \
    "-----END CERTIFICATE-----\n"

#define CERT_RSA_1024                                                    \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIICMjCCAZsCFBhYL8TPQQcPf1mH9N1UBwuQCliiMA0GCSqGSIb3DQEBCwUAMEgx\n" \
    "CzAJBgNVBAYTAlNBMRAwDgYDVQQIDAdDemVjaGlhMQ8wDQYDVQQHDAZQcmFndWUx\n" \
    "FjAUBgNVBAoMDVBydXNhIHRlc3QgQ0EwHhcNMjEwMTI4MTQ1MzQwWhcNMjEwMjI3\n" \
    "MTQ1MzQwWjBoMQswCQYDVQQGEwJTQTEQMA4GA1UECAwHQ3plY2hpYTEPMA0GA1UE\n" \
    "BwwGUHJhZ3VlMR0wGwYDVQQKDBRQcnVzYSB0ZXN0IENBIFNlcnZlcjEXMBUGA1UE\n" \
    "AwwOMTkyLjE2OC44OC4xODkwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBALMW\n" \
    "ZF6o97Ykxl2uJA28P82WLYOA5SkITq2SIw85VY/Fvvq6LSrBgf4JUcU7dSz7EJ05\n" \
    "2LJDIT+PcF3SqV6oWgIUnpE9qwdXii9X1GUQdDwacp38XukH5/nIXzcxsQUHeTN9\n" \
    "B8LzLppYtdfH3ftucuYuCTojHElptjyIAIP52QZLAgMBAAEwDQYJKoZIhvcNAQEL\n" \
    "BQADgYEANrlXfyaETgUNtYeP8IS53ArcomEMH6Z5oPRILPGKua3MFOWweTMtO5xm\n" \
    "GsQPTfNZkaoctzNLt8HI0EKRvvT9H6qqPSOg8E7XtMraYRgxy11ARw0bg4Bqt/E1\n" \
    "NPjiKtSNpqPxQr1nqECpY3hsxdHAT+0wnVZgUjonRKWzurSMxdM=\n"             \
    "-----END CERTIFICATE-----\n"

#define CERT_ECDSA_256                                                   \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIIBqzCCAVECFDaqDu4OcgBqsAwEoIfqEzVmJXJeMAoGCCqGSM49BAMCMEgxCzAJ\n" \
    "BgNVBAYTAlNBMRAwDgYDVQQIDAdDemVjaGlhMQ8wDQYDVQQHDAZQcmFndWUxFjAU\n" \
    "BgNVBAoMDVBydXNhIHRlc3QgQ0EwHhcNMjEwMTI4MTcwODIxWhcNMjEwMjI3MTcw\n" \
    "ODIxWjBoMQswCQYDVQQGEwJTQTEQMA4GA1UECAwHQ3plY2hpYTEPMA0GA1UEBwwG\n" \
    "UHJhZ3VlMR0wGwYDVQQKDBRQcnVzYSB0ZXN0IENBIFNlcnZlcjEXMBUGA1UEAwwO\n" \
    "MTkyLjE2OC44OC4xODkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAROXuPjzbDA\n" \
    "xJ1g8P7kbWxy9p30RvY+sGeN+fjdf7B9MIZx5qJnvwrDOEGZAZEZAJjYVm7cdf+S\n" \
    "abmEcJw7FOE8MAoGCCqGSM49BAMCA0gAMEUCIC20bHOisgL4qn2PtgbQdE2xTWC5\n" \
    "5Z4Bk1yyIlcQNKAxAiEA4hI4wEAdi7tS8XBdk67YKrXbvlvp6x40mtEd/GA/BQ4=\n" \
    "-----END CERTIFICATE-----\n"
// uncomment any of the below defines to enable corresponding cipher suite
//#define USE_CIPHER_RSA_2048
#define USE_CIPHER_RSA_1024
//#define USE_CIPHER_ECC
#if defined(USE_CIPHER_RSA_2048)
const char my_mbedtls_test_cas_pem[] = CERT_ECDSA_256;
#elif defined(USE_CIPHER_RSA_1024)
const char my_mbedtls_test_cas_pem[] = CERT_RSA_1024;
#elif defined(USE_CIPHER_ECC)
const char my_mbedtls_test_cas_pem[] = CERT_RSA_2048;
#else
    #error "error cipher configuration"
#endif

const size_t my_mbedtls_test_cas_pem_len = sizeof(my_mbedtls_test_cas_pem);

static void wui_marlin_client_init(void) {
    wui_marlin_vars = marlin_client_init(); // init the client
    // force update variables when starts
    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF - MARLIN_EVT_MSK_FSM, NULL);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
    if (wui_marlin_vars) {
        wui_marlin_vars->media_LFN = wui_media_LFN;
    }
}

static void sync_with_marlin_server(void) {
    if (wui_marlin_vars) {
        marlin_client_loop();
    } else {
        return;
    }
    osMutexWait(wui_thread_mutex_id, osWaitForever);
    for (int i = 0; i < 4; i++) {
        wui_vars.pos[i] = wui_marlin_vars->pos[i];
    }
    wui_vars.temp_nozzle = wui_marlin_vars->temp_nozzle;
    wui_vars.temp_bed = wui_marlin_vars->temp_bed;
    wui_vars.target_nozzle = wui_marlin_vars->target_nozzle;
    wui_vars.target_bed = wui_marlin_vars->target_bed;
    wui_vars.fan_speed = wui_marlin_vars->fan_speed;
    wui_vars.print_speed = wui_marlin_vars->print_speed;
    wui_vars.flow_factor = wui_marlin_vars->flow_factor;
    wui_vars.print_dur = wui_marlin_vars->print_duration;
    wui_vars.sd_precent_done = wui_marlin_vars->sd_percent_done;
    wui_vars.sd_printing = wui_marlin_vars->sd_printing;
    wui_vars.time_to_end = wui_marlin_vars->time_to_end;
    wui_vars.print_state = wui_marlin_vars->print_state;
    if (marlin_change_clr(MARLIN_VAR_FILENAME)) {
        strlcpy(wui_vars.gcode_name, wui_marlin_vars->media_LFN, FILE_NAME_MAX_LEN);
    }

    osMutexRelease(wui_thread_mutex_id);
}

static void update_eth_changes(void) {
    wui_lwip_link_status(); // checks plug/unplug status and take action
    wui_lwip_sync_gui_lan_settings();
}

static void my_debug(void *ctx, int level,
    const char *file, int line,
    const char *str) {
    ((void)level);

    _dbg("%s:%04d: %s", file, line, str);
}

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    const uint32_t buf_size = 1024;
    char buf[buf_size];
    (void)data;

    _dbg("\nVerifying certificate at depth %d:\n", depth);
    mbedtls_x509_crt_info(buf, buf_size - 1, "  ", crt);
    _dbg("%s", buf);

    if (*flags == 0)
        _dbg("No verification issue for this certificate\n");
    else {
        mbedtls_x509_crt_verify_info(buf, buf_size, "  ! ", *flags);
        _dbg("%s\n", buf);
    }

    return 0;
}

void init_ssl() {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));

    int status;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TRUST CHAIN CONFIGURATION

    mbedtls_x509_crt x509_certificate;
    mbedtls_x509_crt_init(&x509_certificate);

    _dbg("-------------mbedtls starts ----------------------\n");
    mbedtls_x509_crt_init(&x509_certificate);
    status = mbedtls_x509_crt_parse(&x509_certificate, (const unsigned char *)my_mbedtls_test_cas_pem,
        my_mbedtls_test_cas_pem_len);
    if (status < 0) {
        _dbg(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -status);
        goto quite_x509_certificate;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // ENTROPY/RANDOMNESS SOURCE AND PSEUDORANDOM NUMBER GENERATOR (PRNG) CONFIGURATION

    mbedtls_entropy_context entropy_context;
    mbedtls_entropy_init(&entropy_context);

    mbedtls_ctr_drbg_context drbg_context;
    mbedtls_ctr_drbg_init(&drbg_context);

    if ((status = mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy_context, NULL, 0)) != 0) {
        _dbg(" mbedtls_ctr_drbg_seed (-0x%X)\n", -status);
        goto quite_entropy;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TLS CONFIGURATION

    mbedtls_ssl_config ssl_config;
    mbedtls_ssl_config_init(&ssl_config);

    mbedtls_ssl_conf_dbg(&ssl_config, my_debug, NULL);
#ifdef DEBUG
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif
    mbedtls_ssl_conf_verify(&ssl_config, my_verify, NULL);

    if ((status = mbedtls_ssl_config_defaults(&ssl_config,
             MBEDTLS_SSL_IS_CLIENT,
             MBEDTLS_SSL_TRANSPORT_STREAM,
             MBEDTLS_SSL_PRESET_DEFAULT))
        != 0) {
        _dbg(" mbedtls_ssl_config_defaults failed to load default SSL config (-0x%X)\n", -status);
        goto quite_ssl_config;
    }

    // Only use TLS 1.2
#ifndef USE_CIPHER_RSA_1024
    mbedtls_ssl_conf_max_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
#endif
    // set cipher suite to use
#if defined(USE_CIPHER_RSA_1024) || defined(USE_CIPHER_RSA_2048)
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, 0 };
#elif defined(USE_CIPHER_ECC)
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0 };
#endif
    mbedtls_ssl_conf_ciphersuites(&ssl_config, tls_cipher_suites);

    // By limiting ourselves to TLS v1.2 and the previous cipher suites, we can compile mbedTLS without the unused ciphers
    // and reduce its size

    // Load CA certificate
    mbedtls_ssl_conf_ca_chain(&ssl_config, &x509_certificate, NULL);
    // Strictly ensure that certificates are signed by the CA
    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    //    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &drbg_context);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TLS CONTEXT

    mbedtls_ssl_context ssl_context;
    mbedtls_ssl_init(&ssl_context);

    if ((status = mbedtls_ssl_setup(&ssl_context, &ssl_config)) != 0) {
        _dbg(" mbedtls_ssl_setup failed to setup SSL context (-0x%X)\n", -status);
        goto quite_ssl_context;
    }

quite_ssl_context:
    mbedtls_ssl_free(&ssl_context);

quite_ssl_config:
    mbedtls_ssl_config_free(&ssl_config);

quite_entropy:
    mbedtls_ctr_drbg_free(&drbg_context);
    mbedtls_entropy_free(&entropy_context);

quite_x509_certificate:
    mbedtls_x509_crt_free(&x509_certificate);
    // mbedtls_memory_buffer_alloc_status();
    _dbg("*****************mbedtls ends ----------------------\n");
}

void StartWebServerTask(void const *argument) {
    // get settings from ini file
    osDelay(1000);
    if (load_ini_file(&wui_eth_config)) {
        save_eth_params(&wui_eth_config);
    }
    wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&wui_eth_config);
    // mutex for passing marlin variables to tcp thread
    wui_thread_mutex_id = osMutexCreate(osMutex(wui_thread_mutex));
    // marlin client initialization for WUI
    wui_marlin_client_init();
    // LwIP related initalizations
    MX_LWIP_Init(&wui_eth_config);
    httpd_init();
    sntp_client_init();
    init_ssl();
    osDelay(WUI_NETIF_SETUP_DELAY); // wait for all settings to take effect

    for (;;) {
        update_eth_changes();
        sync_with_marlin_server();
        osDelay(1000);
    }
}
