#include "rgw_swift_auth.h"
#include "rgw_rest.h"

#include "common/ceph_crypto.h"
#include "common/Clock.h"

#include "auth/Crypto.h"

using namespace ceph::crypto;

static RGW_SWIFT_Auth_Get rgw_swift_auth_get;

static int build_token(string& os_user, string& key, uint64_t nonce, utime_t& expiration, bufferlist& bl)
{
  ::encode(os_user, bl);
  ::encode(nonce, bl);
  ::encode(expiration, bl);

  bufferptr p(CEPH_CRYPTO_HMACSHA1_DIGESTSIZE);

  char buf[bl.length() * 2 + 1];
  buf_to_hex((const unsigned char *)bl.c_str(), bl.length(), buf);
  RGW_LOG(20) << "build_token token=" << buf << dendl;

  char k[CEPH_CRYPTO_HMACSHA1_DIGESTSIZE];
  memset(k, 0, sizeof(k));
  const char *s = key.c_str();
  for (int i = 0; i < (int)key.length(); i++, s++) {
    k[i % CEPH_CRYPTO_HMACSHA1_DIGESTSIZE] |= *s;
  }
  calc_hmac_sha1(k, sizeof(k), bl.c_str(), bl.length(), p.c_str());

  bl.append(p);

  return 0;

}

static int encode_token(string& os_user, string& key, bufferlist& bl)
{
  uint64_t nonce;

  int ret = get_random_bytes((char *)&nonce, sizeof(nonce));
  if (ret < 0)
    return ret;

  utime_t expiration = ceph_clock_now(g_ceph_context);
  expiration += RGW_SWIFT_TOKEN_EXPIRATION; // 15 minutes

  ret = build_token(os_user, key, nonce, expiration, bl);

  return ret;
}

int rgw_swift_verify_signed_token(const char *token, RGWUserInfo& info)
{
  if (strncmp(token, "AUTH_rgwtk", 10) != 0)
    return -EINVAL;

  token += 10;

  int len = strlen(token);
  if (len & 1) {
    RGW_LOG(0) << "failed to verify token: invalid token length len=" << len << dendl;
    return -EINVAL;
  }

  bufferptr p(len/2);
  int ret = hex_to_buf(token, p.c_str(), len);
  if (ret < 0)
    return ret;

  bufferlist bl;
  bl.append(p);

  bufferlist::iterator iter = bl.begin();

  uint64_t nonce;
  utime_t expiration;
  string os_user;

  try {
    ::decode(os_user, iter);
    ::decode(nonce, iter);
    ::decode(expiration, iter);
  } catch (buffer::error& err) {
    RGW_LOG(0) << "failed to decode token: caught exception" << dendl;
    return -EINVAL;
  }
  if (expiration < ceph_clock_now(g_ceph_context)) {
    RGW_LOG(0) << "old timed out token was used now=" << ceph_clock_now(g_ceph_context) << " token.expiration=" << expiration << dendl;
    return -EPERM;
  }

  if ((ret = rgw_get_user_info_by_swift(os_user, info)) < 0)
    return ret;

  RGW_LOG(10) << "os_user=" << os_user << dendl;

  bufferlist tok;
  ret = build_token(os_user, info.swift_key, nonce, expiration, tok);
  if (ret < 0)
    return ret;

  if (tok.length() != bl.length()) {
    RGW_LOG(0) << "tokens length mismatch: bl.length()=" << bl.length() << " tok.length()=" << tok.length() << dendl;
    return -EPERM;
  }

  if (memcmp(tok.c_str(), bl.c_str(), tok.length()) != 0) {
    char buf[tok.length() * 2 + 1];
    buf_to_hex((const unsigned char *)tok.c_str(), tok.length(), buf);
    RGW_LOG(0) << "WARNING: tokens mismatch tok=" << buf << dendl;
    return -EPERM;
  }

  return 0;
}

void RGW_SWIFT_Auth_Get::execute()
{
  int ret = -EPERM;

  RGW_LOG(20) << "RGW_SWIFT_Auth_Get::execute()" << dendl;

  const char *key = s->env->get("HTTP_X_AUTH_KEY");
  const char *user = s->env->get("HTTP_X_AUTH_USER");
  const char *url_prefix = s->env->get("RGW_SWIFT_URL_PREFIX");
  const char *os_url = s->env->get("RGW_SWIFT_URL");

  string user_str = user;
  RGWUserInfo info;
  bufferlist bl;

  if (!os_url || !url_prefix) {
    RGW_LOG(0) << "server is misconfigured, missing RGW_SWIFT_URL_PREFIX or RGW_SWIFT_URL" << dendl;
    ret = -EINVAL;
    goto done;
  }

  if (!key || !user)
    goto done;

  if ((ret = rgw_get_user_info_by_swift(user_str, info)) < 0)
    goto done;

  if (info.swift_key.compare(key) != 0) {
    RGW_LOG(0) << "RGW_SWIFT_Auth_Get::execute(): bad swift key" << dendl;
    ret = -EPERM;
    goto done;
  }

  CGI_PRINTF(s, "X-Storage-Url: %s/%s/v1/AUTH_rgw\n", os_url, url_prefix);

  if ((ret = encode_token(info.swift_name, info.swift_key, bl)) < 0)
    goto done;

  {
    char buf[bl.length() * 2 + 1];
    buf_to_hex((const unsigned char *)bl.c_str(), bl.length(), buf);

    CGI_PRINTF(s, "X-Storage-Token: AUTH_rgwtk%s\n", buf);
  }

  ret = 204;

done:
  set_req_state_err(s, ret);
  dump_errno(s);
  end_header(s);
}

int RGWHandler_SWIFT_Auth::authorize()
{
  return 0;
}

RGWOp *RGWHandler_SWIFT_Auth::get_op()
{
  RGWOp *op;
  switch (s->op) {
   case OP_GET:
     op = &rgw_swift_auth_get;
     break;
   default:
     return NULL;
  }

  if (op) {
    op->init(s);
  }
  return op;
}

void RGWHandler_SWIFT_Auth::put_op(RGWOp *op)
{
}
