#pragma once

namespace Sockets {
	__m128i guid_to_m128i(GUID src) {
		__m128i ret = _mm_lddqu_si128((__m128i const*)&src);
		return ret;
	}

	GUID m128i_to_guid(__m128i src) {
		GUID ret;
		_mm_storeu_si128((__m128i*)&ret, src);
		return ret;
	}



	template<uint32_t Data1, uint16_t Data2, uint16_t Data3, uint8_t Data4_0, uint8_t Data4_1, uint8_t Data4_2, uint8_t Data4_3, uint8_t Data4_4, uint8_t Data4_5, uint8_t Data4_6, uint8_t Data4_7> __m128i guid_to_m128i() {
		__m128i ret;
		ret.m128i_u32[0] = Data1;/*ui8[0],ui8[1],ui8[2],ui8[3]*/
		ret.m128i_u16[2] = Data2;/*ui8[4],ui8[5]*/
		ret.m128i_u16[3] = Data3;/*ui8[6],ui8[7]*/
		ret.m128i_u8[8] = Data4_0;
		ret.m128i_u8[9] = Data4_1;
		ret.m128i_u8[10] = Data4_2;
		ret.m128i_u8[11] = Data4_3;
		ret.m128i_u8[12] = Data4_4;
		ret.m128i_u8[13] = Data4_5;
		ret.m128i_u8[14] = Data4_6;
		ret.m128i_u8[15] = Data4_7;
		return ret;
	}

	template<typename typeContainer> typeContainer simd_setzero() { typeContainer ret{ 0 }; return ret; }
	template<> decltype(_mm_setzero_si128()) simd_setzero() { return _mm_setzero_si128(); }
	template<> decltype(_mm256_setzero_si256()) simd_setzero() { return _mm256_setzero_si256(); }

	template<typename typeContainer> void simd_store(typeContainer *dst, typeContainer src) { CopyMemory(dst, &src, sizeof(src)); }
	template<> void simd_store(__m128i  *dst, __m128i src) { _mm_store_si128(dst, src); }
	template<> void simd_store(__m256i  *dst, __m256i  src) { _mm256_store_si256(dst, src); }

	template<typename typeContainer>
	class ContainSIMD {
	public:
		typeContainer container{ 0 };
		ContainSIMD() {
			container = simd_setzero<decltype(container)>();
		}

		ContainSIMD(typeContainer init) {
			simd_store(&container, init);
		}
	};

	template<uint32_t Data1, uint16_t Data2, uint16_t Data3, uint8_t Data4_0, uint8_t Data4_1, uint8_t Data4_2, uint8_t Data4_3, uint8_t Data4_4, uint8_t Data4_5, uint8_t Data4_6, uint8_t Data4_7> class guid_sse: public ContainSIMD<__m128i> {
	public:
		guid_sse() {
			container.m128i_u32[0] = Data1;/*ui8[0],ui8[1],ui8[2],ui8[3]*/
			container.m128i_u16[2] = Data2;/*ui8[4],ui8[5]*/
			container.m128i_u16[3] = Data3;/*ui8[6],ui8[7]*/
			container.m128i_u8[8] = Data4_0;
			container.m128i_u8[9] = Data4_1;
			container.m128i_u8[10] = Data4_2;
			container.m128i_u8[11] = Data4_3;
			container.m128i_u8[12] = Data4_4;
			container.m128i_u8[13] = Data4_5;
			container.m128i_u8[14] = Data4_6;
			container.m128i_u8[15] = Data4_7;
		}
		operator const GUID *() const {
			return reinterpret_cast<const GUID *>(&container);
		}
		operator __m128i() {
			return container;
		}
	};



	template<
		uint32_t Data1,
		uint16_t Data2,
		uint16_t Data3,
		uint8_t Data4_0,
		uint8_t Data4_1,
		uint8_t Data4_2,
		uint8_t Data4_3,
		uint8_t Data4_4,
		uint8_t Data4_5,
		uint8_t Data4_6,
		uint8_t Data4_7> __m128i guid_const() {
		__m128i ret;
		return ret;
	};


	typedef guid_sse<0xB5367DF0ui32, 0xCBACui16, 0x11CFui16, 0x95ui8, 0xCAui8, 0x00ui8, 0x80ui8, 0x5Fui8, 0x48ui8, 0xA1ui8, 0x92ui8> M128_TRANSMITFILE;
	typedef guid_sse<0xB5367DF1ui32, 0xCBACui16, 0x11CFui16, 0x95ui8, 0xCAui8, 0x00ui8, 0x80ui8, 0x5Fui8, 0x48ui8, 0xA1ui8, 0x92ui8> M128_ACCEPTEX;
	typedef guid_sse<0xB5367DF2ui32, 0xCBACui16, 0x11CFui16, 0x95ui8, 0xCAui8, 0x00ui8, 0x80ui8, 0x5Fui8, 0x48ui8, 0xA1ui8, 0x92ui8> M128_GETACCEPTEXSOCKADDRS;
	typedef guid_sse<0xD9689DA0ui32, 0x1F90ui16, 0x11D3ui16, 0x99ui8, 0x71ui8, 0x00ui8, 0xC0ui8, 0x4Fui8, 0x68ui8, 0xC8ui8, 0x76ui8> M128_TRANSMITPACKETS;
	typedef guid_sse<0x25A207B9ui32, 0xDDF3ui16, 0x4660ui16, 0x8Eui8, 0xE9ui8, 0x76ui8, 0xE5ui8, 0x8Cui8, 0x74ui8, 0x06ui8, 0x3Eui8> M128_CONNECTEX;
	typedef guid_sse<0x7FDA2E11ui32, 0x8630ui16, 0x436Fui16, 0xA0ui8, 0x31ui8, 0xF5ui8, 0x36ui8, 0xA6ui8, 0xEEui8, 0xC1ui8, 0x57ui8> M128_DISCONNECTEX;
	typedef guid_sse<0xF689D7C8ui32, 0x6F1Fui16, 0x436Bui16, 0x8Aui8, 0x53ui8, 0xE5ui8, 0x4Fui8, 0xE3ui8, 0x51ui8, 0xC3ui8, 0x22ui8> M128_WSARECVMSG;
	typedef guid_sse<0xA441E712ui32, 0x754Fui16, 0x43CAui16, 0x84ui8, 0xA7ui8, 0x0Dui8, 0xEEui8, 0x44ui8, 0xCFui8, 0x60ui8, 0x6Dui8> M128_WSASENDMSG;
//typedef guid_sse<0x18C76F85ui32, 0xDC66ui16, 0x4964ui16, 0x97ui8, 0x2Eui8, 0x23ui8, 0xC2ui8, 0x72ui8, 0x38ui8, 0x31ui8, 0x2Bui8> M128_WSAPOLL;
	typedef guid_sse<0x8509E081ui32, 0x96DDui16, 0x4005ui16, 0xB1ui8, 0x65ui8, 0x9Eui8, 0x2Eui8, 0xE8ui8, 0xC7ui8, 0x9Eui8, 0x3Fui8> M128_MULTIPLE_RIO;

	const M128_TRANSMITFILE m128_TRANSMITFILE;
	const M128_ACCEPTEX m128_ACCEPTEX;
	const M128_GETACCEPTEXSOCKADDRS m128_GETACCEPTEXSOCKADDRS;
	const M128_TRANSMITPACKETS m128_TRANSMITPACKETS;
	const M128_CONNECTEX m128_CONNECTEX;
	const M128_DISCONNECTEX m128_DISCONNECTEX;
	const M128_WSARECVMSG m128_WSARECVMSG;
	const M128_WSASENDMSG m128_WSASENDMSG;
//const M128_WSAPOLL m128_WSAPOLL;
	const M128_MULTIPLE_RIO m128_MULTIPLE_RIO;
};