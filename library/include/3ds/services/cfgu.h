/**
 * @file cfgu.h
 * @brief CFGU (Configuration) Service
 */
#pragma once
#include <3ds/types.h>

/// Configuration region values.
typedef enum
{
	CFG_REGION_JPN = 0, ///< Japan
	CFG_REGION_USA = 1, ///< USA
	CFG_REGION_EUR = 2, ///< Europe
	CFG_REGION_AUS = 3, ///< Australia
	CFG_REGION_CHN = 4, ///< China
	CFG_REGION_KOR = 5, ///< Korea
	CFG_REGION_TWN = 6, ///< Taiwan
} CFG_Region;

/// Configuration language values.
typedef enum
{
	CFG_LANGUAGE_DEFAULT = -1, ///< Use system language in errorInit
	CFG_LANGUAGE_JP,           ///< Japanese
	CFG_LANGUAGE_EN,           ///< English
	CFG_LANGUAGE_FR,           ///< French
	CFG_LANGUAGE_DE,           ///< German
	CFG_LANGUAGE_IT,           ///< Italian
	CFG_LANGUAGE_ES,           ///< Spanish
	CFG_LANGUAGE_ZH,           ///< Simplified Chinese
	CFG_LANGUAGE_KO,           ///< Korean
	CFG_LANGUAGE_NL,           ///< Dutch
	CFG_LANGUAGE_PT,           ///< Portugese
	CFG_LANGUAGE_RU,           ///< Russian
	CFG_LANGUAGE_TW,           ///< Traditional Chinese
} CFG_Language;

// Configuration system model values.
typedef enum
{
	CFG_MODEL_3DS    = 0, ///< Old 3DS (CTR)
	CFG_MODEL_3DSXL  = 1, ///< Old 3DS XL (SPR)
	CFG_MODEL_N3DS   = 2, ///< New 3DS (KTR)
	CFG_MODEL_2DS    = 3, ///< Old 2DS (FTR)
	CFG_MODEL_N3DSXL = 4, ///< New 3DS XL (RED)
	CFG_MODEL_N2DSXL = 5, ///< New 2DS XL (JAN)
} CFG_SystemModel;

/// Initializes CFGU.
Result cfguInit(void);

/// Exits CFGU.
void cfguExit(void);

/**
 * @brief Gets the system's region from secure info.
 * @param region Pointer to output the region to. (see @ref CFG_Region)
 */
Result CFGU_SecureInfoGetRegion(u8* region);

/**
 * @brief Generates a console-unique hash.
 * @param appIDSalt Salt to use.
 * @param hash Pointer to output the hash to.
 */
Result CFGU_GenHashConsoleUnique(u32 appIDSalt, u64* hash);

/**
 * @brief Gets whether the system's region is Canada or USA.
 * @param value Pointer to output the result to. (0 = no, 1 = yes)
 */
Result CFGU_GetRegionCanadaUSA(u8* value);

/**
 * @brief Gets the system's model.
 * @param model Pointer to output the model to. (see @ref CFG_SystemModel)
 */
Result CFGU_GetSystemModel(u8* model);

/**
 * @brief Gets whether the system is a 2DS.
 * @param value Pointer to output the result to. (0 = yes, 1 = no)
 */
Result CFGU_GetModelNintendo2DS(u8* value);

/**
 * @brief Gets a string representing a country code.
 * @param code Country code to use.
 * @param string Pointer to output the string to.
 */
Result CFGU_GetCountryCodeString(u16 code, u16* string);

/**
 * @brief Gets a country code ID from its string.
 * @param string String to use.
 * @param code Pointer to output the country code to.
 */
Result CFGU_GetCountryCodeID(u16 string, u16* code);

/**
 * @brief Checks if NFC (code name: fangate) is supported.
 * @param isSupported pointer to the output the result to.
 */
Result CFGU_IsNFCSupported(bool* isSupported);

/**
 * @brief Gets a config info block with flags = 2.
 * @param size Size of the data to retrieve.
 * @param blkID ID of the block to retrieve.
 * @param outData Pointer to write the block data to.
 */
Result CFGU_GetConfigInfoBlk2(u32 size, u32 blkID, void* outData);

/**
 * @brief Gets a config info block with flags = 4.
 * @param size Size of the data to retrieve.
 * @param blkID ID of the block to retrieve.
 * @param outData Pointer to write the block data to.
 */
Result CFG_GetConfigInfoBlk4(u32 size, u32 blkID, void* outData);

/**
 * @brief Gets a config info block with flags = 8.
 * @param size Size of the data to retrieve.
 * @param blkID ID of the block to retrieve.
 * @param outData Pointer to write the block data to.
 */
Result CFG_GetConfigInfoBlk8(u32 size, u32 blkID, void* outData);

/**
 * @brief Sets a config info block with flags = 4.
 * @param size Size of the data to retrieve.
 * @param blkID ID of the block to retrieve.
 * @param inData Pointer to block data to write.
 */
Result CFG_SetConfigInfoBlk4(u32 size, u32 blkID, const void* inData);

/**
 * @brief Sets a config info block with flags = 8.
 * @param size Size of the data to retrieve.
 * @param blkID ID of the block to retrieve.
 * @param inData Pointer to block data to write.
 */
Result CFG_SetConfigInfoBlk8(u32 size, u32 blkID, const void* inData);


/**
 * @brief Writes the CFG buffer in memory to the savegame in NAND.
 */
Result CFG_UpdateConfigSavegame(void);

/**
 * @brief Gets the system's language.
 * @param language Pointer to write the language to. (see @ref CFG_Language)
 */
Result CFGU_GetSystemLanguage(u8* language);

/**
 * @brief Deletes the NAND LocalFriendCodeSeed file, then recreates it using the LocalFriendCodeSeed data stored in memory.
 */
Result CFGI_RestoreLocalFriendCodeSeed(void);

/**
 * @brief Deletes the NAND SecureInfo file, then recreates it using the SecureInfo data stored in memory.
 */
Result CFGI_RestoreSecureInfo(void);

/**
 * @brief Deletes the "config" file stored in the NAND Config_Savegame.
 */
Result CFGI_DeleteConfigSavefile(void);

/**
 * @brief Formats Config_Savegame.
 */
Result CFGI_FormatConfig(void);

/**
 * @brief Clears parental controls
 */
Result CFGI_ClearParentalControls(void);

/**
 * @brief Verifies the RSA signature for the LocalFriendCodeSeed data already stored in memory.
 */
Result CFGI_VerifySigLocalFriendCodeSeed(void);

/**
 * @brief Verifies the RSA signature for the SecureInfo data already stored in memory.
 */
Result CFGI_VerifySigSecureInfo(void);

/**
 * @brief Gets the system's serial number.
 * @param serial Pointer to output the serial to. (This is normally 0xF)
 */
Result CFGI_SecureInfoGetSerialNumber(u8 *serial);

/**
 * @brief Gets the 0x110-byte buffer containing the data for the LocalFriendCodeSeed.
 * @param data Pointer to output the buffer. (The size must be at least 0x110-bytes)
 */
Result CFGI_GetLocalFriendCodeSeedData(u8 *data);

/**
 * @brief Gets the 64-bit local friend code seed.
 * @param seed Pointer to write the friend code seed to.
 */
Result CFGI_GetLocalFriendCodeSeed(u64* seed);

/**
 * @brief Gets the 0x11-byte data following the SecureInfo signature.
 * @param data Pointer to output the buffer. (The size must be at least 0x11-bytes)
 */
Result CFGI_GetSecureInfoData(u8 *data);

/**
 * @brief Gets the 0x100-byte RSA-2048 SecureInfo signature.
 * @param data Pointer to output the buffer. (The size must be at least 0x100-bytes)
 */
Result CFGI_GetSecureInfoSignature(u8 *data);
