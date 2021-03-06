// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Dacrs developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Why base-58 instead of standard base-64 encoding?
// - Don't want 0OIl characters that look the same in some fonts and
//      could be used to create visually identical looking account numbers.
// - A string with non-alphanumeric characters is not as easily accepted as an account number.
// - E-mail usually won't line-break if there's no punctuation to break at.
// - Double-clicking selects the whole number as one word if it's all alphanumeric.
//
#ifndef DACRS_BASE58_H_
#define DACRS_BASE58_H_

#include "chainparams.h"
#include "key.h"

#include <string>
#include <vector>

/**
 * Encode a byte sequence as a base58-encoded string.
 * pbegin and pend cannot be NULL, unless both are.
 */
string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);

/**
 * Encode a byte vector as a base58-encoded string
 */
string EncodeBase58(const vector<unsigned char>& vch);

/**
 * Decode a base58-encoded string (psz) into a byte vector (vchRet).
 * return true if decoding is successful.
 * psz cannot be NULL.
 */
bool DecodeBase58(const char* psz, vector<unsigned char>& vchRet);

/**
 * Decode a base58-encoded string (str) into a byte vector (vchRet).
 * return true if decoding is successful.
 */
bool DecodeBase58(const string& str, vector<unsigned char>& vchRet);

/**
 * Encode a byte vector into a base58-encoded string, including checksum
 */
string EncodeBase58Check(const vector<unsigned char>& vchIn);

/**
 * Decode a base58-encoded string (psz) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
inline bool DecodeBase58Check(const char* psz, vector<unsigned char>& vchRet);

/**
 * Decode a base58-encoded string (str) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
inline bool DecodeBase58Check(const string& str, vector<unsigned char>& vchRet);

/**
 * Base class for all base58-encoded data
 */
class CBase58Data {
 public:
	bool SetString(const char* psz, unsigned int unVersionBytes = 1);
	bool SetString(const string& str);
	string ToString() const;
	int CompareTo(const CBase58Data& cBase58) const;

	bool operator==(const CBase58Data& b58) const {
		return CompareTo(b58) == 0;
	}
	bool operator<=(const CBase58Data& b58) const {
		return CompareTo(b58) <= 0;
	}
	bool operator>=(const CBase58Data& b58) const {
		return CompareTo(b58) >= 0;
	}
	bool operator<(const CBase58Data& b58) const {
		return CompareTo(b58) < 0;
	}
	bool operator>(const CBase58Data& b58) const {
		return CompareTo(b58) > 0;
	}

 protected:
	CBase58Data();
	void SetData(const vector<unsigned char> &vchVersionIn, const void* pdata, size_t nSize);
	void SetData(const vector<unsigned char> &vchVersionIn, const unsigned char *pbegin, const unsigned char *pend);

	// the version byte(s)
	vector<unsigned char> m_vchVersion;

	// the actually encoded data
	typedef vector<unsigned char, zero_after_free_allocator<unsigned char> > vector_uchar;
	vector_uchar m_vchData;
};

/** base58-encoded Dacrs addresses.
 * Public-key-hash-addresses have version 0 (or 111 testnet).
 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
 * Script-hash-addresses have version 5 (or 196 testnet).
 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
 */
class CDacrsAddress: public CBase58Data {
 public:
	bool Set(const CKeyID &cId);
	bool Set(const CTxDestination &cDest);
	bool IsValid() const;

	CDacrsAddress() {
	}
	CDacrsAddress(const CTxDestination &dest) {
		Set(dest);
	}
	CDacrsAddress(const string& strAddress) {
		SetString(strAddress);
	}
	CDacrsAddress(const char* pszAddress) {
		SetString(pszAddress);
	}

	CTxDestination Get() const;
	bool GetKeyID(CKeyID &keyID) const;
	bool IsScript() const;
};

/**
 * A base58-encoded secret key
 */
class CDacrsSecret: public CBase58Data {
 public:
	void SetKey(const CKey& vchSecret);
	CKey GetKey();
	bool IsValid() const;
	bool SetString(const char* pszSecret);
	bool SetString(const string& strSecret);

	CDacrsSecret(const CKey& vchSecret) {
		SetKey(vchSecret);
	}
	CDacrsSecret() {
	}
};

template<typename K, int Size, CBaseParams::emBase58Type Type> class CDacrsExtKeyBase: public CBase58Data {
 public:
	void SetKey(const K &key) {
		unsigned char vch[Size];
		key.Encode(vch);
		SetData(SysCfg().Base58Prefix(Type), vch, vch + Size);
	}

	K GetKey() {
		K ret;
		ret.Decode(&m_vchData[0], &m_vchData[Size]);
		return ret;
	}

	CDacrsExtKeyBase(const K &key) {
		SetKey(key);
	}

	CDacrsExtKeyBase() {
	}
};

typedef CDacrsExtKeyBase<CExtKey, 74, CBaseParams::EM_EXT_SECRET_KEY> CDacrsExtKey;
typedef CDacrsExtKeyBase<CExtPubKey, 74, CBaseParams::EM_EXT_PUBLIC_KEY> CDacrsExtPubKey;

#endif // DACRS_BASE58_H_
