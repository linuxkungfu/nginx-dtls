#ifndef _NGX_STREAM_PROBE_H_INCLUDED_
#define _NGX_STREAM_PROBE_H_INCLUDED_
#include <ngx_core.h>

#define RTP_HEADER_SIZE 12
#define MAX_COMPRESSIONS_SIZE 255
#define DTLS_RECORD_LAYER_HEADER_SIZE 13
#define DTLS_HANDSHAKE_HEADER_SIZE 12
#define DTLS_RECORD_LAYER_SEQ_NUM_SIZE 6

// STUN message class
enum stun_class
{
  REQUEST          = 0,
  INDICATION       = 1,
  SUCCESS_RESPONSE = 2,
  ERROR_RESPONSE   = 3
};

// STUN message method.
enum stun_method
{
  BINDING = 1
};

// Attribute type.
enum stun_attribute
{
  MAPPED_ADDRESS     = 0x0001,
  USERNAME           = 0x0006,
  MESSAGE_INTEGRITY  = 0x0008,
  ERROR_CODE         = 0x0009,
  UNKNOWN_ATTRIBUTES = 0x000A,
  REALM              = 0x0014,
  NONCE              = 0x0015,
  XOR_MAPPED_ADDRESS = 0x0020,
  PRIORITY           = 0x0024,
  USE_CANDIDATE      = 0x0025,
  SOFTWARE           = 0x8022,
  ALTERNATE_SERVER   = 0x8023,
  FINGERPRINT        = 0x8028,
  ICE_CONTROLLED     = 0x8029,
  ICE_CONTROLLING    = 0x802A,
  NOMINATION         = 0xC001
};

// Authentication result.
enum stun_authentication
{
  OK           = 0,
  UNAUTHORIZED = 1,
  BAD_REQUEST  = 2
};

enum dtls_content_type
{
  change_cipher_spec = 20, 
  alert = 21,
  handshake = 22,
  application_data = 23
};

enum dtls_handshake_type {
  hello_request = 0,
  client_hello = 1, 
  server_hello = 2,
  hello_verify_request = 3,
  certificate = 11,
  server_key_exchange = 12,
  certificate_request = 13,
  server_hello_done = 14,
  certificate_verify = 15,
  client_key_exchange = 16,
  finished = 20
};

struct dtls_version
{
  uint8_t major;
  uint8_t minor;
};

struct dtls_record_layer_header
{
  enum dtls_content_type type;
  struct dtls_version version;
  uint16_t epoch;                        
  uint8_t sequence_number[DTLS_RECORD_LAYER_SEQ_NUM_SIZE];
  uint16_t length;
  uint8_t *data;
};

struct dtls_handshake_header
{
  enum dtls_handshake_type msg_type;
  uint32_t length;                                    // 3 bytes
  uint16_t message_seq;                               // New field
  uint32_t fragment_offset;                           // New field 3bytes
  uint32_t fragment_length;                           // New field 3bytes
  uint* body;
};

struct rtp_header
		{
#if defined(NGX_HAVE_LITTLE_ENDIAN)
			uint8_t csrcCount : 4;
			uint8_t extension : 1;
			uint8_t padding : 1;
			uint8_t version : 2;
			uint8_t payloadType : 7;
			uint8_t marker : 1;
#else
			uint8_t version : 2;
			uint8_t padding : 1;
			uint8_t extension : 1;
			uint8_t csrcCount : 4;
			uint8_t marker : 1;
			uint8_t payloadType : 7;
#endif
			uint16_t sequenceNumber;
			uint32_t timestamp;
			uint32_t ssrc;
		};

struct stun_packet {
  // Passed by argument.
  enum stun_class klass;                        // 2 bytes.
  enum stun_method method;                      // 2 bytes.
  u_char* transaction_id;                 // 12 bytes.
  u_char* data;                                 // Pointer to binary data.
	size_t size;                                  // The full message size (including header).
  // STUN attributes.
  char username[513];                           // Less than 513 bytes.
  uint16_t username_len;                        // username len
  uint32_t priority;                            // 4 bytes unsigned integer.
  uint64_t ice_controlling;                     // 8 bytes unsigned integer.
  uint64_t ice_controlled;                      // 8 bytes unsigned integer.
  uint8_t has_nomination;
  uint32_t nomination;
  uint8_t has_use_candidate;                    // 0 bytes.
  const uint8_t* message_integrity;                   // 20 bytes.
  uint8_t has_fingerprint;                      // 4 bytes.
  struct sockaddr* xor_mapped_address;          // 8 or 20 bytes.
  uint16_t error_code;                          // 4 bytes (no reason phrase).
  char password[64];
  uint16_t password_len;
};

struct ssl_packet {
    /* Pointer to where we are currently reading from */
    const u_char *curr;
    /* Number of bytes remaining */
    size_t remaining;
};

struct ssl_raw_extension {
  /* Raw packet data for the extension */
  struct ssl_packet data;
  /* Set to 1 if the extension is present or 0 otherwise */
  int present;
  /* Set to 1 if we have already parsed the extension or 0 otherwise */
  int parsed;
  /* The type of this extension, i.e. a TLSEXT_TYPE_* value */
  unsigned int type;
  /* Track what order extensions are received in (0-based). */
  size_t received_order;
};

struct dtls_client_hello{
    struct dtls_version legacy_version;
    u_char random[SSL3_RANDOM_SIZE];
    uint8_t session_id_len;
    u_char session_id[SSL_MAX_SSL_SESSION_ID_LENGTH];
    uint8_t dtls_cookie_len;
    unsigned char dtls_cookie[DTLS1_COOKIE_LENGTH];
    uint16_t ciphersuite;
    uint8_t compression_method;
    uint16_t extension_len;
};

struct dtls_server_hello
{
  struct dtls_version server_version;
  u_char random[SSL3_RANDOM_SIZE];
  size_t session_id_len;
  u_char session_id[SSL_MAX_SSL_SESSION_ID_LENGTH];
  uint16_t ciphersuite;
  uint8_t compression_method;
  uint16_t extension_len;
};

ngx_int_t ngx_process_probe(ngx_connection_t *c);
#endif