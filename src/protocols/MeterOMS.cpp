/**
 * OMS (M-Bus) based meter/device support
 * for spec see e.g. http://oms-group.org/fileadmin/pdf/OMS-Spec_Vol2_Primary_v301.pdf
 *
 * @copyright Copyright (c) 2015 - 2023, the volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl2.txt GNU Public License v2
 * @author Matthias Behr <mbehr (a) mcbehr.de>
 * */

#include "protocols/MeterOMS.hpp"
#include <assert.h>
#include <mbus/mbus.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <time.h>

// send_frame: similar to mbus_serial_send_frame from libmbus!

int MeterOMS::OMSHWif::send_frame(mbus_frame *frame) {
	if (!frame)
		return -1;

	unsigned char buff[2048];
	int len;
	if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1) {
		print(log_alert, "send_frame: mbus_frame_pack failed!", "OMS");
		return -2;
	}
	if (len == write(buff, len)) {
		print(log_finest, "send_frame (len=%d) ok", "OMS", len);
		return 0;
	} else {
		print(log_alert, "write != len", "OMS");
		return -3;
	}
}

int MeterOMS::OMSHWif::receive_frame(mbus_frame *frame) {
	unsigned char data[2048];
	int len = 0;
	int rem = 1;
	if (!frame)
		return MBUS_RECV_RESULT_ERROR;
	do {
		if (static_cast<size_t>(rem + len) > sizeof(data)) {
			print(log_debug, "not enough data size! (%d/%d)", "OMS", rem, len);
			return MBUS_RECV_RESULT_ERROR;
		}
		size_t data_size = read(data + len, rem);
		if (data_size > 0) {
			len += data_size;
			rem = mbus_parse(frame, data, len);
			// print(log_finest, "mbus_parse returned %d (%s)", "OMS", rem,
			// mbus_error_str());
		} else {
			print(log_debug, "partial data (%d) received but %d remaining", "OMS", len, rem);
			return MBUS_RECV_RESULT_ERROR;
		}
	} while (rem > 0);
	// todo add check len > SSIZE_MAC-nread
	if (0 == len)
		return MBUS_RECV_RESULT_TIMEOUT;
	if (rem != 0)
		return MBUS_RECV_RESULT_INVALID;

	return MBUS_RECV_RESULT_OK;
}

MeterOMS::OMSSerialHWif::OMSSerialHWif(const std::string &dev, int baudrate)
	: _handle(0), _baudrate(baudrate) {
	_handle = (mbus_handle *)malloc(sizeof(mbus_handle));
	if (!_handle)
		throw vz::VZException("no mem");
	mbus_serial_data *serial_data = (mbus_serial_data *)malloc(sizeof(mbus_serial_data));
	if (!serial_data)
		throw vz::VZException("no mem");
	_handle->max_data_retry = 3;
	_handle->max_search_retry = 1;
	_handle->is_serial = 1;
	_handle->purge_first_frame = MBUS_FRAME_PURGE_M2S;
	_handle->auxdata = serial_data;
	_handle->open = mbus_serial_connect;
	_handle->close = mbus_serial_disconnect;
	_handle->recv = mbus_serial_recv_frame;
	_handle->send = mbus_serial_send_frame;
	_handle->free_auxdata = mbus_serial_data_free;
	_handle->recv_event = 0;
	_handle->send_event = 0;
	_handle->scan_progress = 0;
	_handle->found_event = 0;

	serial_data->device = strdup(dev.c_str());
}

MeterOMS::OMSSerialHWif::~OMSSerialHWif() {
	mbus_serial_data_free(_handle);
	free(_handle);
}

bool MeterOMS::OMSSerialHWif::open() {
	if (mbus_serial_connect(_handle) != 0) {
		print(log_alert, "mbus_connect_serial failed!", "OMS");
		return false;
	}

	if (mbus_serial_set_baudrate(_handle, _baudrate) != 0) {
		print(log_alert, "mbus_serial_set_baudrate failed!", "OMS");
		return false;
	}

	return true;
}

bool MeterOMS::OMSSerialHWif::close() {
	mbus_serial_disconnect(_handle);
	return true;
}

int MeterOMS::OMSSerialHWif::send_frame(mbus_frame *frame) {
	return mbus_serial_send_frame(_handle, frame);
}

int MeterOMS::OMSSerialHWif::receive_frame(mbus_frame *frame) {
	return mbus_serial_recv_frame(_handle, frame);
}

MeterOMS::MeterOMS(const std::list<Option> &options, OMSHWif *hwif)
	: Protocol("oms"), _hwif(hwif), _aes_key(0), _mbus_debug(false), _use_local_time(false),
	  _last_timestamp(0.0) {
	OptionList optlist;
	// todo parse from options for tcp or uart... if (!_hwif) ->
	print(log_debug, "Using libmbus version %s", name().c_str(), mbus_get_current_version());

	// init openssl:
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
	OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL);
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms(); // should be called after init
#else
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);
#endif

	// parse options:
	// expect device and key
	try {
		_device = optlist.lookup_string(options, "device");
	} catch (vz::VZException &e) {
		if (!hwif) {
			print(log_alert, "Missing path or invalid type", name().c_str());
			throw;
		} // else we use the provided one (e.g. from unit tests)
	}

	// optional baudrate, default to 9600:
	int baudrate = 9600;
	try {
		baudrate = optlist.lookup_int(options, "baudrate");
	} catch (vz::VZException &e) {
		// keep default
	}

	if (!hwif) {
		_hwif = new OMSSerialHWif(_device, baudrate);
	}

	// optional mbus_debug, default to false
	try {
		_mbus_debug = optlist.lookup_bool(options, "mbus_debug");
	} catch (vz::VZException &e) {
		// keep default
	}

	try {
		_use_local_time = optlist.lookup_bool(options, "use_local_time");
	} catch (vz::OptionNotFoundException &e) {
		// keep default
	}

	std::string _key;
	try {
		_key = optlist.lookup_string(options, "key");
	} catch (vz::VZException &e) {
		print(log_alert, "Missing path or invalid type", name().c_str());
		throw;
	}
	if (_key.length() != 32) {
		print(log_alert, "Key length needs to be 32!", name().c_str());
		throw vz::VZException("OMS key length error");
	}

	// convert _key into binary _aes_key
	_aes_key = (unsigned char *)malloc(16);
	memset(_aes_key, 0, 16);
	int j = 0;
	for (int i = 0; i < 32; ++i) {
		const char c = _key[i];
		int v = 0;
		if (c >= '0' && c <= '9')
			v = c - '0';
		else if (c >= 'a' && c <= 'f')
			v = 10 + c - 'a';
		else if (c >= 'A' && c <= 'F')
			v = 10 + c - 'A';

		if (i % 2 == 0) {
			v <<= 4;
			_aes_key[j] = v;
		} else {
			_aes_key[j] |= v;
			++j;
		}
	}
}

MeterOMS::~MeterOMS() {
	if (_hwif)
		delete _hwif;

	// openssl cleanup:
	EVP_cleanup();
	ERR_free_strings();

	if (_aes_key)
		free(_aes_key);
}

int MeterOMS::open() {
	if (!_hwif)
		return ERR;
	if (!_hwif->open())
		return ERR;

	return SUCCESS;
}

int MeterOMS::close() {
	if (!_hwif->close())
		return ERR;
	return SUCCESS;
}

ssize_t MeterOMS::read(std::vector<Reading> &rds, size_t n) {
	size_t ret = 0;
	if (!_hwif)
		return 0;

	// we are the slave for MBUS communication

	int expect_frame = 1;
	bool got_SND_NKE = false;
	mbus_frame *frame_ack = mbus_frame_new(MBUS_FRAME_TYPE_ACK);

	while (expect_frame > 0) {

		mbus_frame frame;
		if (MBUS_RECV_RESULT_OK == _hwif->receive_frame(&frame)) {
			--expect_frame;
			print(log_debug,
				  "got valid mbus frame with len=%d, type=%x control=%x controlinfo=%x address=%x",
				  name().c_str(), frame.length1, frame.type, frame.control,
				  frame.control_information, frame.address);
			if ((frame.control & MBUS_CONTROL_MASK_SND_UD) == MBUS_CONTROL_MASK_SND_UD) {
				if (_mbus_debug)
					mbus_frame_print(&frame);
				if (!got_SND_NKE)
					print(log_finest, "got SND_UD without SND_NKE", name().c_str());
				/* for slave to master messages:
			mbus_frame_data reply_data;
			if (mbus_frame_data_parse(&frame2, &reply_data) == -1) {
				print(log_alert, "m-bus data parse error: %s", name().c_str(), mbus_error_str());
			} else {
				char *xml_res;
				if ((xml_res = mbus_frame_data_xml(&reply_data)) == NULL) {
					print(log_alert, "failed to generate xml: %s", name().c_str(),
			mbus_error_str()); } else { print(log_finest, "%s", name().c_str(), xml_res);
				}
			} */

				print(log_finest, "got SND_UD packet", name().c_str());
				// analyse first and ACK only if correct:
				switch (frame.control_information) {
				case 0x5b: // 12 byte CMD to device M-bus 4 Ident 2 Manuf Ver Med Acc Status 2
						   // ConfWord
				{
					// check control word (bytes 10 and 11 (0-based)):
					u_int8_t controlword_low = frame.data[10];
					u_int8_t controlword_high = frame.data[11];
					print(log_debug, "control_word = 0x%x%x", name().c_str(), controlword_high,
						  controlword_low);
					switch (controlword_high & 0xf) {
					case 5: // AES with dynamic initialisation vector
					{
						int nr_enc_16byte_blocks = controlword_low >> 4;
						print(log_debug,
							  "AES with dyn. init. vector for %d 16-byte blocks plus %d "
							  "unencrypted data bytes",
							  name().c_str(), nr_enc_16byte_blocks,
							  frame.length1 - 12 - (16 * nr_enc_16byte_blocks));
						// now do the AES decryption:
						unsigned char iv[16]; // M-field + ID+V+M + 8xacc-nr
						iv[0] = frame.data[4];
						iv[1] = frame.data[5];
						iv[2] = frame.data[0];
						iv[3] = frame.data[1];
						iv[4] = frame.data[2];
						iv[5] = frame.data[3];
						iv[6] = frame.data[6];
						iv[7] = frame.data[7];

						memset(iv + 8, frame.data[8], 8);
						aes_decrypt(frame.data + 12, 16 * nr_enc_16byte_blocks, _aes_key, iv);
						if (_mbus_debug)
							mbus_frame_print(&frame);
						if (frame.length1 <= 14 || frame.data[12] != 0x2f ||
							frame.data[13] != 0x2f) {
							print(log_alert, "encryption sanity check failed", name().c_str());
						} else {
							print(log_finest, "successfully decrypted a frame", name().c_str());
							mbus_frame_data frame_data;
							memset(&frame_data, 0, sizeof(frame_data));
							frame_data.type = MBUS_DATA_TYPE_VARIABLE;
							mbus_data_variable_parse(&frame, &(frame_data.data_var));
							print(log_debug, "got %d data records: %s", name().c_str(),
								  frame_data.data_var.nrecords,
								  _mbus_debug ? mbus_data_variable_xml(&(frame_data.data_var))
											  : "active mbus_debug for detail infos");
							double timeFromMeter = 0.0;
							// go through each record:
							bool ignore_telegram = false;
							for (mbus_data_record *record =
									 (mbus_data_record *)frame_data.data_var.record;
								 record && !ignore_telegram;
								 record = (mbus_data_record *)(record->next)) {
								print(log_debug,
									  "DIF=%.2x, NDIFE=%.2x, DIFE1=%.2x, VIF=%.2x, NVIFE=%.2x "
									  "VIFE1=%.2x VIFE2=%.2x",
									  name().c_str(), record->drh.dib.dif, record->drh.dib.ndife,
									  record->drh.dib.dife[0], record->drh.vib.vif,
									  record->drh.vib.nvife, record->drh.vib.vife[0],
									  record->drh.vib.vife[1]);
								const unsigned char &dif = record->drh.dib.dif;
								const unsigned char &nvife = record->drh.vib.nvife;
								const unsigned char &vife1 = record->drh.vib.vife[0];

								switch (record->drh.vib.vif) {
								case 0x6d: // time
									timeFromMeter = get_record_value(record);
									if (timeFromMeter > 1.0 && (timeFromMeter == _last_timestamp)) {
										// duplicated timestamp received. ignore the remaining
										// telegram as by spec
										ignore_telegram = true;
										print(log_finest,
											  "Ignoring telegram due to duplicated timestamp %f",
											  name().c_str(), timeFromMeter);
									} else {
										if (timeFromMeter > 1.0)
											_last_timestamp = timeFromMeter;
									}
									break;
								case 0x03:
									if (dif == 0x04) { // Obis 1.8.0 Zaehlerstand Energie A+ Wh
										print(log_debug, "Obis 1.8.0 %f %s", name().c_str(),
											  get_record_value(record),
											  mbus_vib_unit_lookup(&(record->drh.vib)));
										if (ret < n) {
											rds[ret].identifier(new ObisIdentifier("1.8.0"));
											rds[ret].value(get_record_value(record));
											if (timeFromMeter > 1.0 && !_use_local_time)
												rds[ret].time_from_double(timeFromMeter);
											else
												rds[ret].time();
											++ret;
										}
									}
									break;
								case 0x83:
									if (dif == 0x04 && nvife == 1 && vife1 == 0x3c) {
										print(log_debug, "Obis 2.8.0 %f %s", name().c_str(),
											  get_record_value(record),
											  mbus_vib_unit_lookup(&(record->drh.vib)));
										if (ret < n) {
											rds[ret].identifier(new ObisIdentifier("2.8.0"));
											rds[ret].value(get_record_value(record));
											if (timeFromMeter > 1.0 && !_use_local_time)
												rds[ret].time_from_double(timeFromMeter);
											else
												rds[ret].time();
											++ret;
										}
									}
									break;
								case 0x2b:
									if (dif == 0x04) {
										print(log_debug, "Obis 1.7.0 %f %s", name().c_str(),
											  get_record_value(record),
											  mbus_vib_unit_lookup(&(record->drh.vib)));
										if (ret < n) {
											rds[ret].identifier(new ObisIdentifier("1.7.0"));
											rds[ret].value(get_record_value(record));
											if (timeFromMeter > 1.0 && !_use_local_time)
												rds[ret].time_from_double(timeFromMeter);
											else
												rds[ret].time();
											++ret;
										}
									}
									break;
								case 0xab:
									if (dif == 0x04 && nvife == 1 && vife1 == 0x3c) {
										print(log_debug, "Obis 2.7.0 %f %s", name().c_str(),
											  get_record_value(record),
											  mbus_vib_unit_lookup(&(record->drh.vib)));
										if (ret < n) {
											rds[ret].identifier(new ObisIdentifier("2.7.0"));
											rds[ret].value(get_record_value(record));
											if (timeFromMeter > 1.0 && !_use_local_time)
												rds[ret].time_from_double(timeFromMeter);
											else
												rds[ret].time();
											++ret;
										}
									}
									break;
								}
							}
							mbus_data_record_free((mbus_data_record *)frame_data.data_var.record);
						}
					} break;
					default:
						print(log_debug, "unsupported MMMM", name().c_str());
						break;
					}
				} break;
				default:
					print(log_debug, "unsupported CI=%x", name().c_str(),
						  frame.control_information);
					break;
				}

				// reply with E5h: MBUS_FRAME_ACK_START
				if (0 != _hwif->send_frame(frame_ack)) {
					print(log_alert, "send_frame failed!", name().c_str());
					expect_frame = 0;
				} else {
					// how to check for further frame?
					// for now assume wait for next one
					// todo!
					expect_frame = 1;
				}
			} else if ((frame.control & MBUS_CONTROL_MASK_SND_NKE) == MBUS_CONTROL_MASK_SND_NKE) {
				got_SND_NKE = true;
				// reply with E5h: MBUS_FRAME_ACK_START
				if (0 != _hwif->send_frame(frame_ack)) {
					print(log_alert, "send_frame failed!", name().c_str());
					expect_frame = 0; // we need to wait for next one
				} else {
					// ack send ok: get next one:
					expect_frame = 1;
				}
			} else {
				print(log_debug, "wrong frame received. waiting for SND_NKE or SND_UD!",
					  name().c_str());
				expect_frame = 0;
			}
		} else {
			expect_frame = 0;
		}

	} // while expect_frame>0
	mbus_frame_free(frame_ack);

	return ret;
}

bool MeterOMS::aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
						   unsigned char *iv) {
	/* no keys in logs!
	printf("iv =");
	for (int i=0; i<16; ++i)
			printf("%.2x", iv[i]);
	printf("\nkey=");
	for (int i=0; i<16; ++i)
			printf("%.2x", key[i]);
	*/

	unsigned char *plaintext = ciphertext; // we decrypt directly into the ciphertext

	EVP_CIPHER_CTX *ctx;

	int len = 0;
	int plaintext_len;

	/* Create and initialise the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		print(log_alert, "EVP_CIPHER_CTX_new failed", name().c_str());
		return false;
	}

	if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
		print(log_alert, "EVP_DecryptInit_ex failed", name().c_str());
		EVP_CIPHER_CTX_free(ctx);
		return false;
	}
	EVP_CIPHER_CTX_set_padding(ctx, 0);

	assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);
	assert(EVP_CIPHER_CTX_key_length(ctx) == 16);

	if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
		print(log_alert, "EVP_DecryptUpdate failed (len=%d)", name().c_str(), len);
		EVP_CIPHER_CTX_free(ctx);
		return false;
	}
	plaintext_len = len;

	if (!EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
		print(log_alert, "EVP_DecryptFinale_ex failed (len=%d)", name().c_str(), len);
		EVP_CIPHER_CTX_free(ctx);
		return false;
	}
	plaintext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len == ciphertext_len;
}

// similar to limbus mbus_data_record_decode: but returning float
double MeterOMS::get_record_value(mbus_data_record *record) const {
	double toRet = 0.0;
	int int_val;
	long long long_long_val;
	struct tm time;

	if (!record)
		return toRet;
	unsigned char vif, vife;
	vif = (record->drh.vib.vif & MBUS_DIB_VIF_WITHOUT_EXTENSION);
	vife = (record->drh.vib.vife[0] & MBUS_DIB_VIF_WITHOUT_EXTENSION);

	switch (record->drh.dib.dif & MBUS_DATA_RECORD_DIF_MASK_DATA) {
	case 0x00: // no data
		break;
	case 0x01: // 1 byte integer (8 bit)
		mbus_data_int_decode(record->data, 1, &int_val);
		toRet = int_val;
		break;
	case 0x02: // 2 byte (16 bit)

		// E110 1100  Time Point (date)
		if (vif == 0x6C) {
			mbus_data_tm_decode(&time, record->data, 2);
			print(log_finest, "time %d.%d.%d", name().c_str(), time.tm_mday, time.tm_mon,
				  time.tm_year);
			// todo convert time to double similar to the other (in ms?)
			// we don't use this time info for now.
		} else // 2 byte integer
		{
			mbus_data_int_decode(record->data, 2, &int_val);
			toRet = int_val;
		}
		break;
	case 0x03: // 3 byte integer (24 bit)
		mbus_data_int_decode(record->data, 3, &int_val);
		toRet = int_val;

		break;
	case 0x04: // 4 byte (32 bit)

		// E110 1101  Time Point (date/time)
		// E011 0000  Start (date/time) of tariff
		// E111 0000  Date and time of battery change
		if ((vif == 0x6D) || ((record->drh.vib.vif == 0xFD) && (vife == 0x30)) ||
			((record->drh.vib.vif == 0xFD) && (vife == 0x70))) {
			mbus_data_tm_decode(&time, record->data, 4);
			// todo
		} else // 4 byte integer
		{
			mbus_data_int_decode(record->data, 4, &int_val);
			toRet = int_val;
		}
		break;
	case 0x05: // 4 Byte Real (32 bit)
		toRet = mbus_data_float_decode(record->data);
		break;
	case 0x06:             // 6 byte integer (48 bit)
		if (vif == 0x6d) { // 48bit time value (CP48)
			struct tm t;
			t.tm_sec = record->data[0] & 0x3f;
			t.tm_min = record->data[1] & 0x3f;
			t.tm_hour = record->data[2] & 0x1f;
			t.tm_mday = record->data[3] & 0x1f;
			t.tm_mon = record->data[4] & 0xf;
			if (t.tm_mon > 0)
				t.tm_mon -= 1; // struct tm is 0-11 based... (months since January)
			t.tm_year =
				100 + (((record->data[3] & 0xe0) >> 5) |
					   ((record->data[4] & 0xf0) >> 1)); // tm_year is number of years since 1900.
			t.tm_isdst = ((record->data[0] & 0x40) == 0x40) ? 1 : 0;
			// check for time invalid at bit 16 (1-based)
			if ((record->data[1] & 0x80) == 0x80) {
				// time invalid!
			} else {
				print(log_finest, "time=%.2d-%.2d-%.2d %.2d:%.2d:%.2d", name().c_str(),
					  1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
				// convert to double (as seconds since 1970-01-01
				toRet = mktime(&t);
			}
		} else {
			mbus_data_long_long_decode(record->data, 6, &long_long_val);
			toRet = long_long_val;
		}
		break;
	case 0x07: // 8 byte integer (64 bit)
		mbus_data_long_long_decode(record->data, 8, &long_long_val);
		toRet = long_long_val;
		break;
		// case 0x08:
	case 0x09: // 2 digit BCD (8 bit)

		int_val = (int)mbus_data_bcd_decode(record->data, 1);
		toRet = int_val;
		break;

	case 0x0A: // 4 digit BCD (16 bit)

		int_val = (int)mbus_data_bcd_decode(record->data, 2);
		toRet = int_val;
		break;

	case 0x0B: // 6 digit BCD (24 bit)

		int_val = (int)mbus_data_bcd_decode(record->data, 3);
		toRet = int_val;
		break;

	case 0x0C: // 8 digit BCD (32 bit)

		int_val = (int)mbus_data_bcd_decode(record->data, 4);
		toRet = int_val;
		break;

	case 0x0E: // 12 digit BCD (48 bit)

		long_long_val = mbus_data_bcd_decode(record->data, 6);
		toRet = long_long_val;
		break;

	default:
		break;
	}
	return toRet;
}
