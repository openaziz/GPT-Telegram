/******************************************************************************
   This implementation is no longer supported but is left here for information.
   That's because Google now only supports POP except with a private client ID.
   Also, it no longer supports OOB requests to Google’s OAuth 2.0 authorization
   endpoint.
 ******************************************************************************/
/* oauth.h -- OAuth 2.0 implementation for XOAUTH2 in SMTP and POP3.
 *
 * Google defined XOAUTH2 for SMTP, and that's what we use here.  If other
 * providers implement XOAUTH2 or some similar OAuth-based SMTP authentication
 * protocol, it should be simple to extend this.
 *
 * OAuth        https://tools.ietf.org/html/rfc6749
 * XOAUTH2      https://developers.google.com/gmail/imap/xoauth2-protocol
 *
 * According to RFC6749 2.1 Client Types, this is a "native application", a
 * "public" client.
 *
 * To summarize the flow:
 *
 * 1. User runs mhlogin which prints a URL the user must visit, and prompts for
 *    a code retrieved from that page.
 *
 * 2. User visits this URL in browser, signs in with some Google account, and
 *    copies and pastes the resulting code back to mhlogin.
 *
 * 3. mhlogin does HTTP POST to Google to exchange the user-provided code for a
 *    short-lived access token and a long-lived refresh token.
 *
 * 4. send uses the access token in SMTP auth if not expired.  If it is expired,
 *    it does HTTP POST to Google including the refresh token and gets back a
 *    new access token (and possibly refresh token).  If the refresh token has
 *    become invalid (e.g. if the user took some reset action on the Google
 *    account), the user must use mhlogin again, then re-run send.
 */

typedef enum {
    /* error loading profile */
    MH_OAUTH_BAD_PROFILE = OK + 1,

    /* error initializing libcurl */
    MH_OAUTH_CURL_INIT,

    /* local error initializing HTTP request */
    MH_OAUTH_REQUEST_INIT,

    /* error executing HTTP POST request */
    MH_OAUTH_POST,

    /* HTTP response body is too big. */
    MH_OAUTH_RESPONSE_TOO_BIG,

    /* Can't process HTTP response body. */
    MH_OAUTH_RESPONSE_BAD,

    /* The authorization server rejected the grant (authorization code or
     * refresh token); possibly the user entered a bad code, or the refresh
     * token has become invalid, etc. */
    MH_OAUTH_BAD_GRANT,

    /* HTTP server indicates something is wrong with our request. */
    MH_OAUTH_REQUEST_BAD,

    /* Attempting to refresh an access token without a refresh token. */
    MH_OAUTH_NO_REFRESH,


    /* requested user not in cred file */
    MH_OAUTH_CRED_USER_NOT_FOUND,

    /* error loading serialized credentials */
    MH_OAUTH_CRED_FILE
} mh_oauth_err_code;

typedef struct mh_oauth_ctx mh_oauth_ctx;

typedef struct mh_oauth_cred mh_oauth_cred;

typedef struct mh_oauth_service_info mh_oauth_service_info;

struct mh_oauth_service_info {
    /* Name of service, so we can search static internal services array
     * and for determining default credential file name. */
    char *name;

    /* Human-readable name of the service; in mh_oauth_ctx::svc this is not
     * another buffer to free, but a pointer to either static SERVICE data
     * (below) or to the name field. */
    char *display_name;

    /* [1] 2.2 Client Identifier, 2.3.1 Client Password */
    char *client_id;
    /* [1] 2.3.1 Client Password */
    char *client_secret;
    /* [1] 3.1 Authorization Endpoint */
    char *auth_endpoint;
    /* [1] 3.1.2 Redirection Endpoint */
    char *redirect_uri;
    /* [1] 3.2 Token Endpoint */
    char *token_endpoint;
    /* [1] 3.3 Access Token Scope */
    char *scope;
};

int mh_oauth_do_xoauth(const char *user, const char *svc,
    unsigned char **oauth_res, size_t *oauth_res_len, FILE *log);
bool mh_oauth_new(mh_oauth_ctx **ctx, const char *svc_name);
void mh_oauth_free(mh_oauth_ctx *ctx);
const char *mh_oauth_svc_display_name(const mh_oauth_ctx *ctx) PURE;
void mh_oauth_log_to(FILE *log, mh_oauth_ctx *ctx);
mh_oauth_err_code mh_oauth_get_err_code(const mh_oauth_ctx *ctx) PURE;
const char *mh_oauth_get_err_string(mh_oauth_ctx *ctx);
const char *mh_oauth_get_authorize_url(mh_oauth_ctx *ctx);
mh_oauth_cred *mh_oauth_authorize(const char *code, mh_oauth_ctx *ctx);
bool mh_oauth_refresh(mh_oauth_cred *cred);
bool mh_oauth_access_token_valid(time_t t, const mh_oauth_cred *cred) PURE;
void mh_oauth_cred_free(mh_oauth_cred *cred);
bool mh_oauth_cred_save(FILE *fp, mh_oauth_cred *cred, const char *user);
mh_oauth_cred *mh_oauth_cred_load(FILE *fp, mh_oauth_ctx *ctx,
    const char *user);
const char *mh_oauth_sasl_client_response(size_t *res_len,
    const char *user, const mh_oauth_cred *cred);
