/*
 *
 *  Copyright (C) 2006 - 2009 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h> 

#include "FLAC/metadata.h"

Boolean 
GetMetadataForFile(void							*thisInterface, 
				   CFMutableDictionaryRef		attributes, 
				   CFStringRef					contentTypeUTI,
				   CFStringRef					pathToFile)
{
	
#pragma unused(thisInterface)
#pragma unused(contentTypeUTI)

	// Convert the path to a format suitable for FLAC
	CFIndex maxLen = CFStringGetMaximumSizeOfFileSystemRepresentation(pathToFile);
	char fileSystemRepresentation [maxLen];

	if(!CFStringGetFileSystemRepresentation(pathToFile, fileSystemRepresentation, maxLen))
		return FALSE;

	FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
	// ENOMEM sux
	if(!chain)
		return FALSE;
	
	// Parse the FLAC metadata
	if(!FLAC__metadata_chain_read(chain, fileSystemRepresentation)) {
		FLAC__metadata_chain_delete(chain);
		return FALSE;
	}
	
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
	// Once again, not enough memory
	if(!iterator) {
		FLAC__metadata_chain_delete(chain);
		return FALSE;
	}
	
	FLAC__metadata_iterator_init(iterator, chain);
	
	do {
		
		FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
		if(NULL == block)
			break;
		
		switch(block->type) {

			case FLAC__METADATA_TYPE_STREAMINFO:
				{
					CFNumberRef sampleRate = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &block->data.stream_info.sample_rate);
					CFDictionarySetValue(attributes, kMDItemAudioSampleRate, sampleRate);
					CFRelease(sampleRate);
					
					CFNumberRef channels = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &block->data.stream_info.channels);
					CFDictionarySetValue(attributes, kMDItemAudioChannelCount, channels);
					CFRelease(channels);
					
					CFNumberRef bitsPerSample = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &block->data.stream_info.bits_per_sample);
					CFDictionarySetValue(attributes, kMDItemBitsPerSample, bitsPerSample);
					CFRelease(bitsPerSample);
					
					unsigned long rawDuration = block->data.stream_info.total_samples / block->data.stream_info.sample_rate;
					CFNumberRef duration = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &rawDuration);
					CFDictionarySetValue(attributes, kMDItemDurationSeconds, duration);
					CFRelease(duration);
				}
				break;
				
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
				{
					char *fieldName = NULL;
					char *fieldValue = NULL;

					for(unsigned commentIndex = 0; commentIndex < block->data.vorbis_comment.num_comments; ++commentIndex) {
						// Parse a vorbis comment (in the NAME=VALUE format) to a pair of values
						// Ignore malformed comments
						if(!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(block->data.vorbis_comment.comments[commentIndex], &fieldName, &fieldValue))
							continue;
						
						CFStringRef key = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, fieldName, kCFStringEncodingASCII, kCFAllocatorMalloc);
						CFStringRef value = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, fieldValue, kCFStringEncodingUTF8, kCFAllocatorMalloc);
						
						if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("ALBUM"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemAlbum, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("ARTIST"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemAuthors, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("COMPOSER"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemComposer, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("GENRE"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemMusicalGenre, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("DATE"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemRecordingYear, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("DESCRIPTION"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemComment, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("TITLE"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemTitle, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("TRACKNUMBER"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemAudioTrackNumber, value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("TRACKTOTAL"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_trackTotal"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("COMPILATION"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_compilation"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("DISCNUMBER"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_discNumber"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("DISCTOTAL"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_discTotal"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("ISRC"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_ISRC"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("MCN"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, CFSTR("org_xiph_flac_MCN"), value);
						else if(kCFCompareEqualTo == CFStringCompare(key, CFSTR("ENCODER"), kCFCompareCaseInsensitive))
							CFDictionarySetValue(attributes, kMDItemAudioEncodingApplication, value);
						
						CFRelease(key), key = NULL;
						CFRelease(value), value = NULL;

						fieldName = fieldValue = NULL;
					}				
				}
				break;
				
			case FLAC__METADATA_TYPE_PICTURE:						break;
			case FLAC__METADATA_TYPE_PADDING:						break;
			case FLAC__METADATA_TYPE_APPLICATION:					break;
			case FLAC__METADATA_TYPE_SEEKTABLE:						break;
			case FLAC__METADATA_TYPE_CUESHEET:						break;
			case FLAC__METADATA_TYPE_UNDEFINED:						break;
		}
	} while(FLAC__metadata_iterator_next(iterator));

	// Tell Spotlight what type of data we indexed
	CFStringRef soundTypes [1];
	soundTypes[0] = CFSTR("Sound");
	
	CFArrayRef mediaTypes = CFArrayCreate(kCFAllocatorDefault, (void *)soundTypes, 1, &kCFTypeArrayCallBacks);
	CFDictionarySetValue(attributes, kMDItemMediaTypes, mediaTypes);
	CFRelease(mediaTypes);
	
	// Clean  up
	if(iterator)
	   FLAC__metadata_iterator_delete(iterator);

	if(chain)
	   FLAC__metadata_chain_delete(chain);

	return TRUE;
}
