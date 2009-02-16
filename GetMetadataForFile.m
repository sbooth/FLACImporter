/*
 *  $Id$
 *
 *  Copyright (C) 2006 - 2007 Stephen F. Booth <me@sbooth.org>
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
#import <Cocoa/Cocoa.h>

#include "FLAC/metadata.h"

Boolean 
GetMetadataForFile(void							*thisInterface, 
				   CFMutableDictionaryRef		attributes, 
				   CFStringRef					contentTypeUTI,
				   CFStringRef					pathToFile)
{
	NSAutoreleasePool				*pool				= nil;
	Boolean							result				= FALSE;
	FLAC__bool						flacResult			= YES;
	FLAC__Metadata_Chain			*chain				= NULL;
	FLAC__Metadata_Iterator			*iterator			= NULL;
	FLAC__StreamMetadata			*block				= NULL;
	char							*fieldName			= NULL;
	char							*fieldValue			= NULL;
	NSString						*key				= nil;
	NSString						*value				= nil;
	unsigned						i					= 0;

	@try  {
		pool = [[NSAutoreleasePool alloc] init];

/*		if(NO == [[NSFileManager defaultManager] fileExistsAtPath:pathToFile]) {
			@throw [NSException exceptionWithName:@"IOException" reason:NSLocalizedStringFromTable(@"The file was not found.", @"Errors", @"") userInfo:nil];
		}*/
		
		chain = FLAC__metadata_chain_new();
		NSCAssert(NULL != chain, NSLocalizedStringFromTable(@"Unable to allocate memory.", @"Errors", @""));
		
		flacResult = FLAC__metadata_chain_read(chain, [(NSString *)pathToFile fileSystemRepresentation]);
		NSCAssert1(YES == flacResult, 
				   NSLocalizedStringFromTable(@"Unable to open the file \"%@\" for reading.", @"Errors", @""),
				   [[NSFileManager defaultManager] displayNameAtPath:(NSString *)pathToFile]);
		
		iterator = FLAC__metadata_iterator_new();
		NSCAssert(NULL != iterator, NSLocalizedStringFromTable(@"Unable to allocate memory.", @"Errors", @""));
		
		FLAC__metadata_iterator_init(iterator, chain);

		do {
			
			block = FLAC__metadata_iterator_get_block(iterator);
			if(NULL == block)
				break;
			
			switch(block->type) {

				case FLAC__METADATA_TYPE_STREAMINFO:
					[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithFloat:(float)block->data.stream_info.sample_rate]
														  forKey:(NSString *)kMDItemAudioSampleRate];
					[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:block->data.stream_info.channels]
														  forKey:(NSString *)kMDItemAudioChannelCount];
					[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:block->data.stream_info.bits_per_sample]
														  forKey:(NSString *)kMDItemBitsPerSample];
					[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithUnsignedLong:block->data.stream_info.total_samples / block->data.stream_info.sample_rate]
														  forKey:(NSString *)kMDItemDurationSeconds];
					break;
					
				case FLAC__METADATA_TYPE_VORBIS_COMMENT:					
					for(i = 0; i < block->data.vorbis_comment.num_comments; ++i) {
						
						if(NO == FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(block->data.vorbis_comment.comments[i], &fieldName, &fieldValue)) {
							// Ignore malformed comments
							continue;
						}
						
						key		= [[NSString alloc] initWithBytesNoCopy:fieldName length:strlen(fieldName) encoding:NSASCIIStringEncoding freeWhenDone:YES];
						value	= [[NSString alloc] initWithBytesNoCopy:fieldValue length:strlen(fieldValue) encoding:NSUTF8StringEncoding freeWhenDone:YES];
					
						if(NSOrderedSame == [key caseInsensitiveCompare:@"ALBUM"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemAlbum];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"ARTIST"])
							[(NSMutableDictionary *)attributes setObject:[NSArray arrayWithObject:value] forKey:(NSString *)kMDItemAuthors];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"COMPOSER"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemComposer];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"GENRE"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemMusicalGenre];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"DATE"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemRecordingYear];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"DESCRIPTION"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemComment];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"TITLE"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemTitle];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"TRACKNUMBER"])
							[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:[value intValue]] forKey:(NSString *)kMDItemAudioTrackNumber];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"TRACKTOTAL"])
							[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:[value intValue]] forKey:@"org_xiph_flac_trackTotal"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"COMPILATION"])
							[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithBool:(BOOL)[value intValue]] forKey:@"org_xiph_flac_compilation"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"DISCNUMBER"])
							[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:[value intValue]] forKey:@"org_xiph_flac_discNumber"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"DISCTOTAL"])
							[(NSMutableDictionary *)attributes setObject:[NSNumber numberWithInt:[value intValue]] forKey:@"org_xiph_flac_discTotal"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"ISRC"])
							[(NSMutableDictionary *)attributes setObject:value forKey:@"org_xiph_flac_ISRC"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"MCN"])
							[(NSMutableDictionary *)attributes setObject:value forKey:@"org_xiph_flac_MCN"];
						else if(NSOrderedSame == [key caseInsensitiveCompare:@"ENCODER"])
							[(NSMutableDictionary *)attributes setObject:value forKey:(NSString *)kMDItemAudioEncodingApplication];
						
						[key release];
						[value release];
						
						fieldName	= NULL;
						fieldValue	= NULL;						
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
		
		
		[(NSMutableDictionary *)attributes setObject:[NSArray arrayWithObject:@"Sound"] forKey:(NSString *)kMDItemMediaTypes];
		
		result = TRUE;
	}
	
	@catch(NSException *exception) {
		NSLog([exception reason]);		
		result = FALSE;
	}
	
	@finally {
		if(NULL != iterator)
			FLAC__metadata_iterator_delete(iterator);
		
		if(NULL != chain)
			FLAC__metadata_chain_delete(chain);
		
		[pool release];
	}
	
	return result;
}
