#include <ngx_stream_probe.h>
#include <ngx_string.h>
#include <ngx_event.h>
#include <ngx_event_udp.h>
#include <ngx_inet.h>
#include <ngx_core.h>
#include <openssl/sha.h>

// https://www.rfc-editor.org/rfc/rfc8446 dtls
static char prefix_probe_1[16] = "probe:";
static char prefix_probe_2[16] = "tools:";
static size_t username_len = 16;
static size_t prefix_len = 6;
const u_char stun_magic_cookie[] = { 0x21, 0x12, 0xA4, 0x42 };

static const uint32_t crc32_table[] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t ngx_get_crc32(const uint8_t* data, size_t size)
{
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t* p = data;

  while (size--)
  {
    crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ ~0U;
}

static const uint8_t* ngx_get_hmac_sha1(const char* key, size_t key_len, const uint8_t* data, size_t len)
{
  static HMAC_CTX* hmac_sha1_ctx = NULL;
  static uint8_t hmac_sha1_buffer[20];
  int ret;
  uint32_t result_len;
  if (hmac_sha1_ctx == NULL) {
    hmac_sha1_ctx = HMAC_CTX_new();
  }
  ret = HMAC_Init_ex(hmac_sha1_ctx, key, key_len, EVP_sha1(), NULL);
  if (ret != 1) {
    return NULL;
  }

  ret = HMAC_Update(hmac_sha1_ctx, data, (int)(len));

  ret = HMAC_Final(hmac_sha1_ctx, (uint8_t*)hmac_sha1_buffer, &result_len);

  if (ret != 1) {
    return NULL;
  }
  return hmac_sha1_buffer;
}

/**
 * Getters below get value in Host Byte Order.
 * Setters below set value in Network Byte Order.
 */
static uint8_t ngx_get_1_byte(const uint8_t* data, size_t i)
{
  return data[i];
}

static uint16_t ngx_get_2_bytes(const uint8_t* data, size_t i)
{
  return ((uint16_t)data[i + 1]) | (((uint16_t)data[i]) << 8);
}

static uint32_t ngx_get_3_bytes(const uint8_t* data, size_t i)
{
  return ((uint32_t)data[i + 2]) | (((uint32_t)data[i + 1]) << 8) | (((uint32_t)data[i]) << 16);
}

static uint32_t ngx_get_4_bytes(const uint8_t* data, size_t i)
{
  return ((uint32_t)data[i + 3]) | (((uint32_t)data[i + 2]) << 8) |
         (((uint32_t)data[i + 1]) << 16) | (((uint32_t)data[i]) << 24);
}

static uint64_t ngx_get_8_bytes(const uint8_t* data, size_t i)
{
  return (((uint64_t)ngx_get_4_bytes(data, i)) << 32) | ngx_get_4_bytes(data, i + 4);
}

static void ngx_set_1_byte(uint8_t* data, size_t i, uint8_t value)
{
  data[i] = value;
}

static void ngx_set_2_bytes(uint8_t* data, size_t i, uint16_t value)
{
  data[i + 1] = (uint8_t)value;
  data[i]     = (uint8_t)(value >> 8);
}

static void ngx_set_3_bytes(uint8_t* data, size_t i, uint32_t value)
{
  data[i + 2] = (uint8_t)value;
  data[i + 1] = (uint8_t)(value >> 8);
  data[i]     = (uint8_t)(value >> 16);
}

static void ngx_set_4_bytes(uint8_t* data, size_t i, uint32_t value)
{
  data[i + 3] = (uint8_t)value;
  data[i + 2] = (uint8_t)(value >> 8);
  data[i + 1] = (uint8_t)(value >> 16);
  data[i]     = (uint8_t)(value >> 24);
}

static void ngx_set_8_bytes(uint8_t* data, size_t i, uint64_t value)
{
  data[i + 7] = (uint8_t)value;
  data[i + 6] = (uint8_t)(value >> 8);
  data[i + 5] = (uint8_t)(value >> 16);
  data[i + 4] = (uint8_t)(value >> 24);
  data[i + 3] = (uint8_t)(value >> 32);
  data[i + 2] = (uint8_t)(value >> 40);
  data[i + 1] = (uint8_t)(value >> 48);
  data[i]     = (uint8_t)(value >> 56);
}

static uint16_t ngx_pad_to_4_bytes_from_2_bytes(uint16_t size)
{
  // If size is not multiple of 32 bits then pad it.
  if (size & 0x03)
    return (size & 0xFFFC) + 4;
  else
    return size;
}

// static uint32_t ngx_pad_to_4_bytes_from_4_bytes(uint32_t size)
// {
//   // If size is not multiple of 32 bits then pad it.
//   if (size & 0x03)
//     return (size & 0xFFFFFFFC) + 4;
//   else
//     return size;
// }

ngx_int_t ngx_stun_parse(struct stun_packet* packet, const uint8_t* data, size_t len)
{
  // Start looking for attributes after STUN header (Byte #20).
  size_t pos = 20;
  // Flags (positions) for special MESSAGE-INTEGRITY and FINGERPRINT attributes.
  u_int8_t has_message_integrity = 0;
  u_int8_t has_fingerprint = 0;
  size_t fingerprint_attr_pos = 0; // Will point to the beginning of the attribute.
  uint32_t fingerprint = 0;      // Holds the value of the FINGERPRINT attribute.
  uint16_t msg_type;
  uint16_t msg_length;
  uint16_t msg_method;
  uint16_t msg_class;

  /*
    The message type field is decomposed further into the following
      structure:

      0                 1
      2  3  4 5 6 7 8 9 0 1 2 3 4 5
          +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
          |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
          |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
          +--+--+-+-+-+-+-+-+-+-+-+-+-+-+

      Figure 3: Format of STUN Message Type Field

      Here the bits in the message type field are shown as most significant
      (M11) through least significant (M0).  M11 through M0 represent a 12-
      bit encoding of the method.  C1 and C0 represent a 2-bit encoding of
      the class.
  */

		// Get type field.
		msg_type = ngx_get_2_bytes(data, 0);

		// Get length field.
		msg_length = ngx_get_2_bytes(data, 2);

		// length field must be total size minus header's 20 bytes, and must be multiple of 4 Bytes.
		if (((size_t)msg_length != len - 20) || ((msg_length & 0x03) != 0))
		{
			return NGX_ERROR;
		}

		// Get STUN method.
		msg_method = (msg_type & 0x000f) | ((msg_type & 0x00e0) >> 1) | ((msg_type & 0x3E00) >> 2);

		// Get STUN class.
		msg_class = ((data[0] & 0x01) << 1) | ((data[1] & 0x10) >> 4);

    packet->klass = msg_class;
    packet->method = msg_method;
    packet->transaction_id = (u_char*)(data + 8);
		/*
      STUN Attributes

      After the STUN header are zero or more attributes.  Each attribute
      MUST be TLV encoded, with a 16-bit type, 16-bit length, and value.
      Each STUN attribute MUST end on a 32-bit boundary.  As mentioned
      above, all fields in an attribute are transmitted most significant
      bit first.

          0                   1                   2                   3
          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |         Type                  |            Length             |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                         Value (variable)                ....
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		*/

		// Ensure there are at least 4 remaining bytes (attribute with 0 length).
		while (pos + 4 <= len)
		{
      enum stun_attribute attr_type;
      uint16_t attr_length;
      const uint8_t* attr_value_pos;
			// Get the attribute type.
			attr_type = (enum stun_attribute)(ngx_get_2_bytes(data, pos));

			// Get the attribute length.
			attr_length = ngx_get_2_bytes(data, pos + 2);

			// Ensure the attribute length is not greater than the remaining size.
			if ((pos + 4 + attr_length) > len)
			{
				return NGX_ERROR;
			}

			// FINGERPRINT must be the last attribute.
			if (has_fingerprint)
			{
				return NGX_ERROR;
			}

			// After a MESSAGE-INTEGRITY attribute just FINGERPRINT is allowed.
			if (has_message_integrity && attr_type != FINGERPRINT)
			{
				return NGX_ERROR;
			}

			attr_value_pos = data + pos + 4;

			switch (attr_type)
			{
				case USERNAME:
				{
          if (attr_length >= sizeof(packet->username)) {
            return NGX_ERROR;
          }
          ngx_memcpy(packet->username, attr_value_pos, attr_length);
          packet->username[attr_length] = 0x00;
          packet->username_len = attr_length;
					break;
				}
				case PRIORITY:
				{
					// Ensure attribute length is 4 bytes.
					if (attr_length != 4)
					{
						return NGX_ERROR;
					}
          packet->priority = ngx_get_4_bytes(attr_value_pos, 0);
					break;
				}
				case ICE_CONTROLLING:
				{
					// Ensure attribute length is 8 bytes.
					if (attr_length != 8)
					{
						return NGX_ERROR;
					}
          packet->ice_controlling = ngx_get_8_bytes(attr_value_pos, 0);
					break;
				}

				case ICE_CONTROLLED:
				{
					// Ensure attribute length is 8 bytes.
					if (attr_length != 8)
					{
						return NGX_ERROR;
					}
          packet->ice_controlled = ngx_get_8_bytes(attr_value_pos, 0);
					break;
				}

				case USE_CANDIDATE:
				{
					// Ensure attribute length is 0 bytes.
					if (attr_length != 0)
					{
						return NGX_ERROR;
					}
          packet->has_use_candidate = 1;
					break;
				}

				case NOMINATION:
        {
          // Ensure attribute length is 4 bytes.
          if (attr_length != 4)
          {
              return NGX_ERROR;
          }

          packet->has_nomination = 1;
          packet->nomination = ngx_get_4_bytes(attr_value_pos, 0);
          break;
        }

				case MESSAGE_INTEGRITY:
				{
					// Ensure attribute length is 20 bytes.
					if (attr_length != 20)
					{
						return NGX_ERROR;
					}

					has_message_integrity = 1;
          packet->message_integrity = attr_value_pos;
					break;
				}

				case FINGERPRINT:
				{
					// Ensure attribute length is 4 bytes.
					if (attr_length != 4)
					{
						return NGX_ERROR;
					}

					has_fingerprint     = 1;
					fingerprint_attr_pos = pos;
					fingerprint        = ngx_get_4_bytes(attr_value_pos, 0);
          packet->has_fingerprint = 1;
					break;
				}

				case ERROR_CODE:
				{
          uint8_t error_class;
          uint8_t error_number;
					// Ensure attribute length >= 4bytes.
					if (attr_length < 4)
					{
						return NGX_ERROR;
					}

					error_class  = ngx_get_1_byte(attr_value_pos, 2);
					error_number = ngx_get_1_byte(attr_value_pos, 3);
					packet->error_code   = (uint16_t)(((uint16_t)error_class) * 100 + error_number);
					break;
				}
				default:;
			}

			// Set next attribute position.
			pos = (size_t)(ngx_pad_to_4_bytes_from_2_bytes((uint16_t)(pos + 4 + attr_length)));
		}

		// Ensure current position matches the total length.
		if (pos != len)
		{
			return NGX_ERROR;
		}

		// If it has FINGERPRINT attribute then verify it.
		if (has_fingerprint)
		{
			// Compute the CRC32 of the received packet up to (but excluding) the
			// FINGERPRINT attribute and XOR it with 0x5354554e.
			uint32_t computed_fingerprint = ngx_get_crc32(data, fingerprint_attr_pos) ^ 0x5354554e;

			// Compare with the FINGERPRINT value in the packet.
			if (fingerprint != computed_fingerprint)
			{
				return NGX_ERROR;
			}
		}
		return NGX_OK;
	}

static ngx_int_t ngx_stun_serialize(struct stun_packet* packet, uint8_t* buffer, size_t len)
{
  // Some useful variables.
  uint16_t username_padded_len = 0;
  uint16_t xor_mapped_address_padded_len = 0;
  uint8_t add_xor_mapped_address;
  uint8_t add_error_code;
  uint8_t add_message_integrity;
  uint8_t add_fingerprint = 0; // Do always.
  uint16_t type_field;
  size_t pos = 20;
  uint8_t* attr_value;
  uint8_t code_class;
  uint8_t code_number;
  const uint8_t* computed_message_integrity;

  add_xor_mapped_address =
    ((packet->xor_mapped_address != NULL) && packet->method == BINDING &&
      packet->klass == SUCCESS_RESPONSE) ? 1 : 0;
  add_error_code = ((packet->error_code != 0) && packet->klass == ERROR_RESPONSE);
  add_message_integrity = (packet->klass != ERROR_RESPONSE && packet->password_len != 0);
 
  packet->data = buffer;
  packet->size = 20;
  if (packet->username_len > 0)
  {
    username_padded_len = ngx_pad_to_4_bytes_from_2_bytes((uint16_t)packet->username_len);
    packet->size += 4 + packet->username_len;
  }

  if (packet->priority != 0u)
    packet->size += 4 + 4;

  if (packet->ice_controlling != 0u)
    packet->size += 4 + 8;

  if (packet->ice_controlled != 0u)
    packet->size += 4 + 8;

  if (packet->has_use_candidate)
    packet->size += 4;

  if (add_xor_mapped_address)
  {
    switch (packet->xor_mapped_address->sa_family)
    {
      case AF_INET:
      {
        xor_mapped_address_padded_len = 8;
        packet->size += 4 + 8;
        break;
      }
      case AF_INET6:
      {
        xor_mapped_address_padded_len = 20;
        packet->size += 4 + 20;
        break;
      }
      default:
      {
        add_xor_mapped_address = 0;
      }
    }
  }
  if (add_error_code)
    packet->size += 4 + 4;

  if (add_message_integrity)
    packet->size += 4 + 20;

  if (add_fingerprint)
    packet->size += 4 + 4;
  if (packet->size > len) {
    return NGX_ABORT;
  }
  // Merge class and method fields into type.
  type_field = ((uint16_t)(packet->method) & 0x0f80) << 2;

  type_field |= ((uint16_t)(packet->method) & 0x0070) << 1;
  type_field |= ((uint16_t)(packet->method) & 0x000f);
  type_field |= ((uint16_t)(packet->klass) & 0x02) << 7;
  type_field |= ((uint16_t)(packet->klass) & 0x01) << 4;

  // Set type field.
  ngx_set_2_bytes(buffer, 0, type_field);
  // Set length field.
  ngx_set_2_bytes(buffer, 2, (uint16_t)(packet->size) - 20);
  // Set magic cookie.
  ngx_memcpy(buffer + 4, stun_magic_cookie, 4);
  // Set TransactionId field.
  ngx_memcpy(buffer + 8, packet->transaction_id, 12);
  // Update the transaction ID pointer.
  packet->transaction_id = buffer + 8;
		
  // Add atributes.

  // Add USERNAME.
  if (username_padded_len != 0u)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(USERNAME));
    ngx_set_2_bytes(buffer, pos + 2, (uint16_t)(packet->username_len));
    ngx_memcpy(buffer + pos + 4, packet->username, packet->username_len);
    pos += 4 + username_padded_len;
  }

  // Add PRIORITY.
  if (packet->priority != 0u)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)PRIORITY);
    ngx_set_2_bytes(buffer, pos + 2, 4);
    ngx_set_4_bytes(buffer, pos + 4, packet->priority);
    pos += 4 + 4;
  }

  // Add ICE-CONTROLLING.
  if (packet->ice_controlling != 0u)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(ICE_CONTROLLING));
    ngx_set_2_bytes(buffer, pos + 2, 8);
    ngx_set_8_bytes(buffer, pos + 4, packet->ice_controlling);
    pos += 4 + 8;
  }

  // Add ICE-CONTROLLED.
  if (packet->ice_controlled != 0u)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(ICE_CONTROLLED));
    ngx_set_2_bytes(buffer, pos + 2, 8);
    ngx_set_8_bytes(buffer, pos + 4, packet->ice_controlled);
    pos += 4 + 8;
  }

  // Add USE-CANDIDATE.
  if (packet->has_use_candidate)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(USE_CANDIDATE));
    ngx_set_2_bytes(buffer, pos + 2, 0);
    pos += 4;
  }

  // Add XOR-MAPPED-ADDRESS
  if (add_xor_mapped_address)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(XOR_MAPPED_ADDRESS));
    ngx_set_2_bytes(buffer, pos + 2, xor_mapped_address_padded_len);

    attr_value = buffer + pos + 4;

    switch (packet->xor_mapped_address->sa_family)
    {
      case AF_INET:
      {
        // Set first byte to 0.
        attr_value[0] = 0;
        // Set inet family.
        attr_value[1] = 0x01;
        // Set port and XOR it.
        ngx_memcpy(attr_value + 2, &(((const struct sockaddr_in*)(packet->xor_mapped_address))->sin_port), 2);
        attr_value[2] ^= stun_magic_cookie[0];
        attr_value[3] ^= stun_magic_cookie[1];
        // Set address and XOR it.
        ngx_memcpy(attr_value + 4, &((const struct sockaddr_in*)(packet->xor_mapped_address))->sin_addr.s_addr, 4);
        attr_value[4] ^= stun_magic_cookie[0];
        attr_value[5] ^= stun_magic_cookie[1];
        attr_value[6] ^= stun_magic_cookie[2];
        attr_value[7] ^= stun_magic_cookie[3];

        pos += 4 + 8;

        break;
      }

      case AF_INET6:
      {
        // Set first byte to 0.
        attr_value[0] = 0;
        // Set inet family.
        attr_value[1] = 0x02;
        // Set port and XOR it.
        ngx_memcpy(attr_value + 2, &((const struct sockaddr_in6*)(packet->xor_mapped_address))->sin6_port, 2);
        attr_value[2] ^= stun_magic_cookie[0];
        attr_value[3] ^= stun_magic_cookie[1];
        // Set address and XOR it.
        ngx_memcpy(attr_value + 4, &((const struct sockaddr_in6*)(packet->xor_mapped_address))->sin6_addr.s6_addr, 16);
        attr_value[4] ^= stun_magic_cookie[0];
        attr_value[5] ^= stun_magic_cookie[1];
        attr_value[6] ^= stun_magic_cookie[2];
        attr_value[7] ^= stun_magic_cookie[3];
        attr_value[8] ^= packet->transaction_id[0];
        attr_value[9] ^= packet->transaction_id[1];
        attr_value[10] ^= packet->transaction_id[2];
        attr_value[11] ^= packet->transaction_id[3];
        attr_value[12] ^= packet->transaction_id[4];
        attr_value[13] ^= packet->transaction_id[5];
        attr_value[14] ^= packet->transaction_id[6];
        attr_value[15] ^= packet->transaction_id[7];
        attr_value[16] ^= packet->transaction_id[8];
        attr_value[17] ^= packet->transaction_id[9];
        attr_value[18] ^= packet->transaction_id[10];
        attr_value[19] ^= packet->transaction_id[11];

        pos += 4 + 20;

        break;
      }
    }
  }

  // Add ERROR-CODE.
  if (add_error_code)
  {
    ngx_set_2_bytes(buffer, pos, (uint16_t)(ERROR_CODE));
    ngx_set_2_bytes(buffer, pos + 2, 4);

    code_class = (uint8_t)(packet->error_code / 100);
    code_number = (uint8_t)(packet->error_code) - (code_class * 100);

    ngx_set_2_bytes(buffer, pos + 4, 0);
    ngx_set_1_byte(buffer, pos + 6, code_class);
    ngx_set_1_byte(buffer, pos + 7, code_number);
    pos += 4 + 4;
  }

  // Add MESSAGE-INTEGRITY.
  if (add_message_integrity)
  {
    // Ignore FINGERPRINT.
    if (add_fingerprint)
      ngx_set_2_bytes(buffer, 2, (uint16_t)(packet->size - 20 - 8));

    // Calculate the HMAC-SHA1 of the packet according to MESSAGE-INTEGRITY rules.
    computed_message_integrity = ngx_get_hmac_sha1(packet->password, packet->password_len, buffer, pos);

    ngx_set_2_bytes(buffer, pos, (uint16_t)(MESSAGE_INTEGRITY));
    ngx_set_2_bytes(buffer, pos + 2, 20);
    ngx_memcpy(buffer + pos + 4, computed_message_integrity, 20);

    // Update the pointer.
    packet->message_integrity = buffer + pos + 4;
    pos += 4 + 20;

    // Restore length field.
    if (add_fingerprint)
      ngx_set_2_bytes(buffer, 2, (uint16_t)(packet->size - 20));
  }
  else
  {
    // Unset the pointer (if it was set).
    packet->message_integrity = NULL;
  }

  // Add FINGERPRINT.
  if (add_fingerprint)
  {
    // Compute the CRC32 of the packet up to (but excluding) the FINGERPRINT
    // attribute and XOR it with 0x5354554e.
    uint32_t computed_fingerprint = ngx_get_crc32(buffer, pos) ^ 0x5354554e;

    ngx_set_2_bytes(buffer, pos, (uint16_t)(FINGERPRINT));
    ngx_set_2_bytes(buffer, pos + 2, 4);
    ngx_set_4_bytes(buffer, pos + 4, computed_fingerprint);
    pos += 4 + 4;

    // Set flag.
    packet->has_fingerprint = 1;
  }
  else
  {
    packet->has_fingerprint = 0;
  }
  return NGX_OK;
}
static ngx_int_t ngx_server_hello_serialize(struct dtls_server_hello* sh, uint8_t* buffer, size_t len)
{
  ngx_int_t pos = 0;
  ngx_set_1_byte(buffer, pos++, sh->server_version.major);
  ngx_set_1_byte(buffer, pos++, sh->server_version.minor);
  ngx_memcpy(buffer + pos, sh->random, SSL3_RANDOM_SIZE);
  pos += SSL3_RANDOM_SIZE;
  ngx_set_1_byte(buffer, pos++, sh->session_id_len);
  if (sh->session_id_len > 0) {
    ngx_memcpy(buffer + pos, sh->session_id, sh->session_id_len);
    pos += sh->session_id_len;
  }
  ngx_set_2_bytes(buffer, pos, sh->ciphersuite);
  pos += 2;
  ngx_set_1_byte(buffer, pos++, sh->compression_method);
  ngx_set_2_bytes(buffer, pos, sh->extension_len);
  pos += 2;
  return pos;
}
static ngx_int_t ngx_handshake_serialize(struct dtls_handshake_header* hs, struct dtls_server_hello* sh, uint8_t* buffer, size_t len)
{
  ngx_int_t pos = 0;
  ngx_set_1_byte(buffer, pos++, hs->msg_type);
  ngx_set_3_bytes(buffer, pos, hs->length);
  pos += 3;
  ngx_set_2_bytes(buffer, pos, hs->message_seq);
  pos += 2;
  ngx_set_3_bytes(buffer, pos, hs->fragment_offset);
  pos += 3; 
  
  switch(hs->msg_type) {
    case server_hello:{
      ngx_int_t server_hello_len = ngx_server_hello_serialize(sh, buffer + pos + 3, len - (pos + 3));
      ngx_set_3_bytes(buffer, pos, server_hello_len);
      ngx_set_3_bytes(buffer, 1, server_hello_len);
      pos += 3;
      pos += server_hello_len;
      return pos;
    }
    default:
      return 0;
  }
  return 0;
}
static ngx_int_t ngx_record_layer_serialize(struct dtls_record_layer_header* record, struct dtls_server_hello* sh, uint8_t* buffer, size_t len)
{
  ngx_int_t pos = 0;

  ngx_set_1_byte(buffer, pos++, record->type);
  ngx_set_1_byte(buffer, pos++, record->version.major);
  ngx_set_1_byte(buffer, pos++, record->version.minor);
  ngx_set_2_bytes(buffer, pos, record->epoch);
  pos += 2;
  ngx_memcpy(buffer + pos, record->sequence_number, DTLS_RECORD_LAYER_SEQ_NUM_SIZE);
  pos += DTLS_RECORD_LAYER_SEQ_NUM_SIZE;
  
  switch (record->type) {
    case handshake: {
      ngx_int_t handshake_len = ngx_handshake_serialize((struct dtls_handshake_header*)record->data, sh, buffer + pos + 2, len - (pos + 2));
      ngx_set_2_bytes(buffer, pos, (uint16_t)handshake_len);
      pos += 2;
      pos += handshake_len;
      return pos;
    }
    default:
      return 0;

  }
  return 0;
}

static ngx_int_t ngx_is_rtp_probe(const u_char* data, size_t len)
{
  const struct rtp_header* header;
  header = (const struct rtp_header*)data;

  return (
    (len >= RTP_HEADER_SIZE) &&
    // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
    (data[0] > 127 && data[0] < 192) &&
    // RTP Version must be 2.
    (header->version == 2)
  ) ? NGX_OK : NGX_ERROR;
  
}

static ngx_int_t ngx_is_stun_probe(const u_char* data, size_t len)
{
  return (
    // STUN headers are 20 bytes.
    (len >= 20) &&
    // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
    (data[0] < 3) &&
    // Magic cookie must match.
    (data[4] == stun_magic_cookie[0]) && (data[5] == stun_magic_cookie[1]) &&
    (data[6] == stun_magic_cookie[2]) && (data[7] == stun_magic_cookie[3])
  ) ? NGX_OK : NGX_ERROR;
}

static ngx_int_t ngx_is_dtls_probe(const uint8_t* data, size_t len)
{
  return (
    // Minimum DTLS record length is 13 bytes.
    (len >= 13) &&
    // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
    (data[0] > 19 && data[0] < 64)
  ) ? NGX_OK : NGX_ERROR;
}

static ngx_int_t ngx_reply_probe(ngx_connection_t *c, void* data, size_t data_len)
{
  // ngx_iovec_t    vec;
  // struct iovec   iovs[1];
  // iovs[0].iov_base = data;
  // iovs[0].iov_len = data_len;
  // vec.iovs = iovs;
  // vec.nalloc = 1;
  ngx_udp_send(c, data, data_len);
  return NGX_OK;
}
static ngx_int_t ngx_process_udp_probe(ngx_connection_t *c, u_char* p, size_t len) 
{
  char* username = NULL;
  ngx_int_t ret = NGX_ERROR;
  if (ngx_memcmp(prefix_probe_1, (const char *)p, prefix_len) != 0 && ngx_memcmp(prefix_probe_2, (const char *)p, prefix_len) != 0) {
    return NGX_DONE;
  }
  username = ngx_palloc(c->pool, username_len + 1);
  if (username == NULL) {
    return NGX_DONE;
  }
  if (*(p + prefix_len) <= 1) {
    *(p + prefix_len) = 0;
    ret = NGX_OK;
  } else {
    *(p + prefix_len) = *(p + prefix_len) - 1;
    ret = NGX_DONE;
  }
  // *(p + prefix_len) = 0xFF;
  *(p + prefix_len + 1) = *(p + prefix_len + 1) + 1;
  ngx_memcpy(username, p + prefix_len + 2, username_len);
  username[username_len] = 0x00;
  ngx_reply_probe(c, p, len);
  ngx_log_error(NGX_LOG_INFO, c->log, 0, "[probe][%s]process udp packet len:%d", username, len);
  ngx_pfree(c->pool, username);
  return ret;
}
static ngx_int_t ngx_process_rtp_probe(ngx_connection_t *c, u_char* p, size_t len) 
{
  char* username = NULL;
  if (ngx_is_rtp_probe(p, len) != NGX_OK) {
    return NGX_DONE;
  }
  u_char* payload = p + RTP_HEADER_SIZE;
  if (ngx_memcmp(prefix_probe_1, (const char *)payload, prefix_len) != 0 && ngx_memcmp(prefix_probe_2, (const char *)payload, prefix_len) != 0) {
    return NGX_DONE;
  }
  *(payload + prefix_len) = 0xFF;
  *(payload + prefix_len + 1) = *(payload + prefix_len + 1) + 1;
  ngx_reply_probe(c, p, len);
  username = ngx_palloc(c->pool, username_len + 1);
  if (username == NULL) {
    return NGX_DONE;
  }
  ngx_memcpy(username, payload + prefix_len + 2, username_len);
  username[username_len] = 0x00;
  ngx_log_error(NGX_LOG_INFO, c->log, 0, "[probe][%s]process rtp packet len:%d", username, len);
  ngx_pfree(c->pool, username);
  return NGX_OK;
}
static ngx_int_t ngx_process_stun_probe(ngx_connection_t *c, u_char* p, size_t len) 
{
  u_char* buffer;
  if (ngx_is_stun_probe(p, len) != NGX_OK) {
    return NGX_DONE;
  }
  struct stun_packet* packet = ngx_pcalloc(c->pool, sizeof(struct stun_packet));
  if (packet == NULL) {
    ngx_log_error(NGX_LOG_ERR, c->log, 0, "ngx_pcalloc stun_packet failed");
    return NGX_DONE;
  }
  if (ngx_stun_parse(packet, p, len) != NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, c->log, 0, "parse stun failed");
    return NGX_DONE;
  }
  if (packet->klass != REQUEST) {
    return NGX_DONE;
  }
  if (ngx_memcmp(packet->transaction_id + 4, prefix_probe_1, prefix_len) != 0 && ngx_memcmp(packet->transaction_id + 4, prefix_probe_2, prefix_len) != 0) {
    return NGX_DONE;
  }
  packet->transaction_id[10] = 0xFF;
  packet->transaction_id[11] = packet->transaction_id[11] + 1;
  packet->klass = SUCCESS_RESPONSE;
  buffer = ngx_palloc(c->pool, len);
  ngx_stun_serialize(packet, buffer, len);
  ngx_reply_probe(c, buffer, packet->size);
  ngx_log_error(NGX_LOG_INFO, c->log, 0, "[probe][%s]process stun packet len:%d", packet->username, len);
  ngx_pfree(c->pool, buffer);
  ngx_pfree(c->pool, packet);
  return NGX_OK;
}
static ngx_int_t ngx_process_dtls_probe(ngx_connection_t *c, u_char* p, size_t len) 
{
  struct dtls_record_layer_header* record = NULL;
  struct dtls_handshake_header* header = NULL;
  struct dtls_client_hello* ch = NULL;
  struct dtls_server_hello* sh = NULL;
  char* username = NULL;
  int pos = 0;
  if (ngx_is_dtls_probe(p, len) != NGX_OK) {
    return NGX_DONE;
  }
  record = ngx_pcalloc(c->pool, sizeof(struct dtls_record_layer_header));
  if (record == NULL) {
    return NGX_DONE;
  }

  record->type = ngx_get_1_byte(p, pos++);
  if (record->type != handshake) {
    ngx_pfree(c->pool, record);
    return NGX_DONE;
  }
  record->version.major = ngx_get_1_byte(p, pos++);
  record->version.minor = ngx_get_1_byte(p, pos++);
  record->epoch = ngx_get_2_bytes(p, pos);
  pos += 2;
  ngx_memcpy(record->sequence_number, p + pos, DTLS_RECORD_LAYER_SEQ_NUM_SIZE);
  pos += DTLS_RECORD_LAYER_SEQ_NUM_SIZE;
  record->length = ngx_get_2_bytes(p, pos);
  pos += 2; 
  if (record->length != 53) {
    ngx_pfree(c->pool, record);
    return NGX_DONE; 
  }
  header = ngx_pcalloc(c->pool, sizeof(struct dtls_handshake_header));
  if (header == NULL) {
    ngx_pfree(c->pool, record);
    return NGX_DONE;
  }
  record->data = (void *)header;
  header->msg_type = ngx_get_1_byte(p, pos++);
  header->length = ngx_get_3_bytes(p, pos);
  pos += 3;
  header->message_seq = ngx_get_2_bytes(p, pos);
  pos += 2;
  header->fragment_offset = ngx_get_3_bytes(p, pos);
  pos += 3; 
  header->fragment_length = ngx_get_3_bytes(p, pos);
  pos += 3;
  if (header->fragment_length != 41 || header->msg_type != client_hello) {
    ngx_pfree(c->pool, record);
    ngx_pfree(c->pool, header);
    return NGX_DONE;
  }

  // header->data = p + DTLS_HANDSHAKE_HEADER_SIZE;
  ch = ngx_pcalloc(c->pool, sizeof(struct dtls_client_hello));
  if (ch == NULL) {
    ngx_pfree(c->pool, record);
    ngx_pfree(c->pool, header);
    return NGX_DONE;
  }
  header->body = (void *)client_hello;
  ch->legacy_version.major = ngx_get_1_byte(p, pos++);
  ch->legacy_version.minor = ngx_get_1_byte(p, pos++);
  ngx_memcpy(ch->random, p + pos, SSL3_RANDOM_SIZE);
  if (ngx_memcmp(ch->random + 8, prefix_probe_1, prefix_len) != 0 && ngx_memcmp(ch->random + 8, prefix_probe_2, prefix_len) != 0) {
    ngx_pfree(c->pool, ch);
    ngx_pfree(c->pool, record);
    ngx_pfree(c->pool, header);
    return NGX_DONE;
  }
  pos += SSL3_RANDOM_SIZE;
  ch->session_id_len = ngx_get_1_byte(p, pos++);
  if (ch->session_id_len > 0) {
    ngx_memcpy(ch->session_id, p + pos, ch->session_id_len);
    pos += ch->session_id_len;
  }
  ch->dtls_cookie_len = ngx_get_1_byte(p, pos++);
  if (ch->dtls_cookie_len > 0) {
    ngx_memcpy(ch->dtls_cookie, p + pos, ch->dtls_cookie_len);
    pos += ch->dtls_cookie_len;
  }
  ch->ciphersuite = ngx_get_2_bytes(p, pos);
  pos += 2;
  ch->compression_method = ngx_get_1_byte(p, pos++);
  ch->extension_len = ngx_get_2_bytes(p, pos);
  pos += 2;
  record->version.minor = 0xfd;
  sh = ngx_pcalloc(c->pool, sizeof(struct dtls_server_hello));
  if (sh == NULL) {
    ngx_pfree(c->pool, ch);  
    ngx_pfree(c->pool, header);
    ngx_pfree(c->pool, record);
    return NGX_DONE;
  }
  username = ngx_palloc(c->pool, username_len + 1);
  if (username == NULL) {
    ngx_pfree(c->pool, ch);  
    ngx_pfree(c->pool, header);
    ngx_pfree(c->pool, record);
    return NGX_DONE;
  }
  // timestamp(4) + seqnum(4) + prefix(6) + ttl(1) + nodenum(1) + username(16)
  ngx_memcpy(username, ch->random + 8 + prefix_len + 1 + 1, username_len);
  username[username_len] = 0x00;
  sh->server_version.major = ch->legacy_version.major;
  sh->server_version.minor = 0xfd;
  ngx_memcpy(sh->random, ch->random, SSL3_RANDOM_SIZE);
  sh->random[8 + prefix_len] = 0xFF;
  sh->random[8 + prefix_len + 1] = ch->random[8 + prefix_len + 1] + 1;
  sh->ciphersuite = ch->ciphersuite;
  sh->compression_method = ch->compression_method;
  sh->session_id_len = ch->session_id_len;
  sh->extension_len = ch->extension_len;
  ngx_memcpy(sh->session_id, ch->session_id, sh->session_id_len);
  header->msg_type = server_hello;
  pos = ngx_record_layer_serialize(record, sh, p, len);
  if (pos > 0) {
    ngx_reply_probe(c, p, pos);
    ngx_log_error(NGX_LOG_INFO, c->log, 0, "[probe][%s]process dtls packet len:%d", username, len);
  } else {
    ngx_pfree(c->pool, sh);  
    ngx_pfree(c->pool, ch);  
    ngx_pfree(c->pool, header);
    ngx_pfree(c->pool, record);
    ngx_pfree(c->pool, username);
    return NGX_DONE;
  }
  ngx_pfree(c->pool, sh);  
  ngx_pfree(c->pool, ch);  
  ngx_pfree(c->pool, header);
  ngx_pfree(c->pool, record);
  ngx_pfree(c->pool, username);
  return NGX_OK;
}
ngx_int_t ngx_process_probe(ngx_connection_t *c)
{
  size_t len;
  u_char* p;
  ngx_buf_t *buffer;
  ngx_int_t ret;
  if (c->type == SOCK_DGRAM) {
    buffer = c->buffer;
    if (buffer == NULL && c->udp != NULL) {
      buffer = c->udp->buffer;
    }
  } else {
    // to do
    return NGX_DONE;
  }
  if (buffer == NULL) {
    return NGX_DONE; 
  }
  p = buffer->pos;
  len = buffer->last - p;
  switch(len) {
    case 24:
      ret = ngx_process_udp_probe(c, p, len);
      break;
    case 36:
      ret = ngx_process_rtp_probe(c, p, len);
      break;
    case 40:
      ret = ngx_process_stun_probe(c, p, len);
      break;
    case 66:
      ret = ngx_process_dtls_probe(c, p, len);
      break;
    default:
      ret = NGX_DONE;
      break;
  }

  if (NGX_OK == ret) {
    ngx_pfree(c->pool, buffer);
    if (c->type == SOCK_DGRAM) {
      c->buffer = NULL;
      c->udp->buffer = NULL;
    }
  }
  return ret;
}