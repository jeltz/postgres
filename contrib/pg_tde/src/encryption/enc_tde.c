#include "pg_tde_defines.h"

#include "postgres.h"

#include "access/pg_tde_tdemap.h"
#include "encryption/enc_tde.h"
#include "encryption/enc_aes.h"
#include "storage/bufmgr.h"

#define AES_BLOCK_SIZE 		        16
#define NUM_AES_BLOCKS_IN_BATCH     200
#define DATA_BYTES_PER_AES_BATCH    (NUM_AES_BLOCKS_IN_BATCH * AES_BLOCK_SIZE)

#ifdef ENCRYPTION_DEBUG
static void
iv_prefix_debug(const char *iv_prefix, char *out_hex)
{
	for (int i = 0; i < 16; ++i)
	{
		sprintf(out_hex + i * 2, "%02x", (int) *(iv_prefix + i));
	}
	out_hex[32] = 0;
}
#endif

/*
 * Encrypts/decrypts `data` with a given `key`. The result is written to `out`.
 *
 * start_offset: is the absolute location of start of data in the file.
 *
 * We require data_len to be a multiple of 16 which XLOG_BLCKSZ is always a
 * multiple of.
 */
void
pg_tde_crypt(const char *iv_prefix, uint32 start_offset, const char *data, uint32 data_len, char *out, InternalKey *key, void **ctxPtr, const char *context)
{
	const uint64 aes_start_block = start_offset / AES_BLOCK_SIZE;
	const uint64 aes_end_block = (start_offset + data_len) / AES_BLOCK_SIZE;
	uint32		data_index = 0;

	Assert(start_offset % AES_BLOCK_SIZE == 0);
	Assert(data_len % AES_BLOCK_SIZE == 0);

	/* do max NUM_AES_BLOCKS_IN_BATCH blocks at a time */
	for (uint64 batch_start_block = aes_start_block; batch_start_block < aes_end_block; batch_start_block += NUM_AES_BLOCKS_IN_BATCH)
	{
		unsigned char enc_key[DATA_BYTES_PER_AES_BATCH];
		uint32		current_batch_bytes;
		uint64		batch_end_block = Min(batch_start_block + NUM_AES_BLOCKS_IN_BATCH, aes_end_block);

		Aes128EncryptedZeroBlocks(ctxPtr, key->key, iv_prefix, batch_start_block, batch_end_block, enc_key);

#ifdef ENCRYPTION_DEBUG
		{
			char		ivp_debug[33];

			iv_prefix_debug(iv_prefix, ivp_debug);
			ereport(LOG,
					(errmsg("%s: Data_Len: %u, batch_start_block: %lu, batch_end_block: %lu, IV prefix: %s",
							context ? context : "", data_len, batch_start_block, batch_end_block, ivp_debug)));
		}
#endif

		current_batch_bytes = (batch_end_block - batch_start_block) * AES_BLOCK_SIZE;

		for (uint32 i = 0; i < current_batch_bytes; i++, data_index++)
		{
			out[data_index] = data[data_index] ^ enc_key[i];
		}
	}
}
