/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


//
// Some notes on mp3:
// A mp3 Frame is always samples/samplerate seconds in length
//
//
//
// What we need are raw 16 bit stereo samples at 44100 Hz which results in 588 samples
// per block (2352 bytes: 32*588 bit). 1 second are 75 blocks.
//

#include "k3bmaddecoder.h"

#include <kurl.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>

#include <qstring.h>
#include <qfile.h>
#include <qvaluevector.h>

#include <stdlib.h>
#include <cmath>
#include <cstdlib>

#include <config.h>

#ifdef HAVE_LIBID3
#include <id3/misc_support.h>
#endif


typedef unsigned char k3b_mad_char;

int K3bMadDecoder::MaxAllowedRecoverableErrors = 10;


class K3bMadDecoder::Private
{
public:
  Private()
    : madStructuresInitialized(false)
#ifdef HAVE_LIBID3
      , id3Tag(0)
#endif
  {
  }

  QValueVector<unsigned long long> seekPositions;

  k3b_mad_char* inputBuffer;

  bool madStructuresInitialized;

  mad_stream*   madStream;
  mad_frame*    madFrame;
  mad_header*   madHeader;
  mad_synth*    madSynth;
  mad_timer_t*  madTimer;

  bool bEndOfInput;
  bool bInputError;
  bool bOutputFinished;

  char* outputBuffer;
  char* outputPointer;
  char* outputBufferEnd;

  QFile inputFile;

#ifdef HAVE_LIBID3
  ID3_Tag* id3Tag;
#endif

  // the first frame header for technical info
  mad_header firstHeader;
  bool vbr;
};


K3bMadDecoder::K3bMadDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new Private();

  d->inputBuffer  = new k3b_mad_char[INPUT_BUFFER_SIZE];

  d->madStream = new mad_stream;
  d->madFrame  = new mad_frame;
  d->madHeader = new mad_header;
  d->madSynth  = new mad_synth;
  d->madTimer  = new mad_timer_t;
}


K3bMadDecoder::~K3bMadDecoder()
{
  cleanup();

  delete [] d->inputBuffer;

  delete d->madStream;
  delete d->madFrame;
  delete d->madHeader;
  delete d->madSynth;
  delete d->madTimer;

  delete d;
}


QString K3bMadDecoder::metaInfo( const QString& tag )
{
#ifdef HAVE_LIBID3
  // use id3 stuff
  if( !d->id3Tag ) {
    d->id3Tag = new ID3_Tag( QFile::encodeName(filename()) );
  }


  char* str = 0;
  if( tag == "Title" )
    str = ID3_GetTitle( d->id3Tag );
  else if( tag == "Artist" )
    str = ID3_GetArtist( d->id3Tag );
  else if( tag == "Album" )
    str = ID3_GetAlbum( d->id3Tag );
  else if( tag == "Year" )
    str = ID3_GetYear( d->id3Tag );
  else if( tag == "Songwriter" )
    str = ID3_GetLyricist( d->id3Tag );
  else if( tag == "Genre" )
    str = ID3_GetGenre( d->id3Tag );
  else if( tag == "Comment" )
    str = ID3_GetComment( d->id3Tag );

  if( str != 0 ) {
    QString s(str);
    delete [] str;
    return s;
  }

  return QString::null;

#else
  return K3bAudioDecoder::metaInfo( tag );
#endif
}


void K3bMadDecoder::initMadStructures()
{
  if( !d->madStructuresInitialized ) {
    mad_stream_init( d->madStream );
    mad_timer_reset( d->madTimer );
    mad_frame_init( d->madFrame );
    mad_header_init( d->madHeader );
    mad_synth_init( d->madSynth );

    d->madStructuresInitialized = true;
  }
}


bool K3bMadDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
  initDecoderInternal();
  frames = countFrames();
  if( frames > 0 ) {
    // we convert mono to stereo all by ourselves. :)
    ch = 2;
    samplerate = d->firstHeader.samplerate;
    return true;
  }
  else
    return false;
}


bool K3bMadDecoder::initDecoderInternal()
{
  cleanup();

  d->bOutputFinished = false;
  d->bEndOfInput = false;
  d->bInputError = false;
    
  d->inputFile.setName( filename() );
  if( !d->inputFile.open( IO_ReadOnly ) ) {
    kdError() << "(K3bMadDecoder) could not open file " << filename() << endl;
    return false;
  }

  memset( d->inputBuffer, 0, INPUT_BUFFER_SIZE );

  initMadStructures();

  return true;
}

// streams data from file into stream
void K3bMadDecoder::madStreamBuffer()
{
  if( d->bEndOfInput )
    return;

  /* The input bucket must be filled if it becomes empty or if
   * it's the first execution of the loop.
   */
  if( d->madStream->buffer == 0 || d->madStream->error == MAD_ERROR_BUFLEN ) {
    long readSize, remaining;
    unsigned char* readStart;

    if( d->madStream->next_frame != 0 ) {
      remaining = d->madStream->bufend - d->madStream->next_frame;
      memmove( d->inputBuffer, d->madStream->next_frame, remaining );
      readStart = d->inputBuffer + remaining;
      readSize = INPUT_BUFFER_SIZE - remaining;
    }
    else {
      readSize  = INPUT_BUFFER_SIZE;
      readStart = d->inputBuffer;
      remaining = 0;
    }
			
    // Fill-in the buffer. 
    Q_LONG result = d->inputFile.readBlock( (char*)readStart, readSize );
    if( result < 0 ) {
      kdDebug() << "(K3bMadDecoder) read error on bitstream)" << endl;
      d->bInputError = true;
      return;
    }
    else if( result == 0 ) {
      kdDebug() << "(K3bMadDecoder) end of input stream" << endl;
      d->bEndOfInput = true;
    }
    else {
      // Pipe the new buffer content to libmad's stream decoder facility.
      mad_stream_buffer( d->madStream, d->inputBuffer, result + remaining );
      d->madStream->error = MAD_ERROR_NONE;
    }
  }
}


unsigned long K3bMadDecoder::countFrames()
{
  unsigned long frames = 0;
  bool error = false;
  d->vbr = false;
  bool bFirstHeaderSaved = false;

  d->seekPositions.clear();

  while( !error && decodeNextHeader() ) {

    if( !bFirstHeaderSaved ) {
      bFirstHeaderSaved = true;
      d->firstHeader = *d->madHeader;
    }
    else {
      if( d->madHeader->bitrate != d->firstHeader.bitrate )
	d->vbr = true;

      if( mad_timer_compare( d->firstHeader.duration, d->madHeader->duration ) ) {
	// The Mp3 standard needs every frame to have the same duration
	kdDebug() << "(K3bMadDecoder) frame len differs: old: " 
		  << d->firstHeader.duration.seconds << ":" << d->firstHeader.duration.fraction
		  << " new: " << d->madHeader->duration.seconds << ":" << d->madHeader->duration.fraction << endl;
	error = true;
      }
    }

    //
    // position in stream: postion in file minus the not yet used buffer
    //
    unsigned long long seekPos = d->inputFile.at() - (d->madStream->bufend - d->madStream->this_frame + 1);

    // save the number of bytes to be read to decode i-1 frames at position i
    // in other words: when seeking to seekPos the next decoded frame will be i
    d->seekPositions.append( seekPos );
  }

  if( !d->bInputError && !error ) {
    // we need the length of the track to be multible of frames (1/75 second)
    float seconds = (float)d->madTimer->seconds + (float)d->madTimer->fraction/(float)MAD_TIMER_RESOLUTION;
    frames = (unsigned long)ceil(seconds * 75.0);
    kdDebug() << "(K3bMadDecoder) length of track " << seconds << endl;
  }

  cleanup();

  return frames;
}


int K3bMadDecoder::decodeInternal( char* _data, int maxLen )
{
  d->outputBuffer = _data;
  d->outputBufferEnd = d->outputBuffer + maxLen;
  d->outputPointer = d->outputBuffer;

  bool bOutputBufferFull = false;

  while( !bOutputBufferFull && !d->bEndOfInput ) {

    // a mad_synth contains of the data of one mad_frame
    // one mad_frame represents a mp3-frame which is always 1152 samples
    // for us that means we need 4*1152 bytes of output buffer for every frame
    // since one sample has 16 bit
    if( d->outputBufferEnd - d->outputPointer < 4*1152 ) {
      bOutputBufferFull = true;
    }
    else if( madDecodeNextFrame() ) {
      // 
      // Once decoded the frame is synthesized to PCM samples. No errors
      // are reported by mad_synth_frame();
      //
      mad_synth_frame( d->madSynth, d->madFrame );
      
      // this fills the output buffer
      if( !createPcmSamples( d->madSynth ) ) {
	return -1;
      }
    }
    else if( d->bInputError ) {
      return -1;
    }
  }

  // flush the output buffer
  size_t buffersize = d->outputPointer - d->outputBuffer;

  return buffersize;
}


bool K3bMadDecoder::madDecodeNextFrame()
{
  if( d->bInputError || d->bEndOfInput ) {
    return false;
  }

  madStreamBuffer();

  if( mad_frame_decode( d->madFrame, d->madStream ) ) {
    if( d->madStream->error == MAD_ERROR_BUFLEN ) {
      return madDecodeNextFrame();
    }
    if( MAD_RECOVERABLE( d->madStream->error )  ) {
      kdDebug() << "(K3bMadDecoder) recoverable frame level error ("
		<< mad_stream_errorstr(d->madStream) << ")" << endl;

      return madDecodeNextFrame();
    }
    else {
      kdDebug() << "(K3bMadDecoder) unrecoverable frame level error ("
		<< mad_stream_errorstr(d->madStream) << endl;
      
      return false;
    }
  }
  else {
    mad_timer_add( d->madTimer, d->madFrame->header.duration );
    return true;
  }
}


bool K3bMadDecoder::decodeNextHeader()
{
  if( d->bInputError || d->bEndOfInput ) {
    return false;
  }

  madStreamBuffer();

  if( mad_header_decode( d->madHeader, d->madStream ) ) {
    if( d->madStream->error == MAD_ERROR_BUFLEN ) {
      return decodeNextHeader();
    }
    else if( MAD_RECOVERABLE( d->madStream->error ) ) {
      kdDebug() << "(K3bMadDecoder) recoverable frame level error ("
		<< mad_stream_errorstr(d->madStream) << ")" << endl;
      
      return decodeNextHeader();
    }
    else {
      kdDebug() << "(K3bMadDecoder) unrecoverable frame level error ("
		<< mad_stream_errorstr(d->madStream) << endl;
      
      return false;
    }
  }
  else {
    mad_timer_add( d->madTimer, d->madHeader->duration );
    return true;
  }
}


unsigned short K3bMadDecoder::linearRound( mad_fixed_t fixed )
{
  // round
  fixed += (1L << ( MAD_F_FRACBITS - 16 ));

  // clip
  if( fixed >= MAD_F_ONE - 1 )
    fixed = MAD_F_ONE - 1;
  else if( fixed < -MAD_F_ONE )
    fixed = -MAD_F_ONE;

  // quatisize
  return fixed >> (MAD_F_FRACBITS + 1 - 16 );
}


bool K3bMadDecoder::createPcmSamples( mad_synth* synth )
{
  unsigned short nsamples = synth->pcm.length;

  // this should not happen since we only decode if the
  // output buffer has enough free space
  if( d->outputBufferEnd - d->outputPointer < nsamples*4 ) {
    kdDebug() <<  "(K3bMadDecoder) buffer overflow!" << endl;
    return false;
  }

  // now create the output
  for( int i = 0; i < nsamples; i++ ) {

    /* Left channel */
    unsigned short sample = linearRound( synth->pcm.samples[0][i] );
    *(d->outputPointer++) = (sample >> 8) & 0xff;
    *(d->outputPointer++) = sample & 0xff;
    
    /* Right channel. If the decoded stream is monophonic then
     * the right output channel is the same as the left one.
     */
    if( synth->pcm.channels == 2 )
      sample = linearRound( synth->pcm.samples[1][i] );
      
    *(d->outputPointer++) = (sample >> 8) & 0xff;
    *(d->outputPointer++) = sample & 0xff;
  } // pcm conversion
  
  return true;
}


void K3bMadDecoder::cleanup()
{
#ifdef HAVE_LIBID3
  delete d->id3Tag;
  d->id3Tag = 0;
#endif

  if( d->inputFile.isOpen() ) {
    kdDebug() << "(K3bMadDecoder) cleanup at offset: " 
	      << ( d->inputFile.at() - (d->madStream->bufend - d->madStream->this_frame + 1) ) 
	      << " with size: " << d->inputFile.size() << endl;
    d->inputFile.close();
  }

  if( d->madStructuresInitialized ) {
    mad_frame_finish( d->madFrame );
    mad_header_finish( d->madHeader );
    mad_synth_finish( d->madSynth );
    mad_stream_finish( d->madStream );
  }

  d->madStructuresInitialized = false;
}


bool K3bMadDecoder::seekInternal( const K3b::Msf& pos )
{
  //
  // we need to reset the complete mad stuff 
  //
  if( !initDecoderInternal() )
    return false;

  //
  // search a position
  // This is all hacking, I don't really know what I am doing here... ;)
  //
  double mp3FrameSecs = static_cast<double>(d->firstHeader.duration.seconds) 
    + static_cast<double>(d->firstHeader.duration.fraction) / static_cast<double>(MAD_TIMER_RESOLUTION);

  double posSecs = static_cast<double>(pos.totalFrames()) / 75.0;

  // seekPosition to seek after frame i
  unsigned int frame = static_cast<unsigned int>( posSecs / mp3FrameSecs );

  // Rob said: 29 frames is the theoretically max frame reservoir limit (whatever that means...)
  // it seems that mad needs at most 29 frames to get ready
  unsigned int frameReservoirProtect = ( frame > 29 ? 29 : frame );

  frame -= frameReservoirProtect;

  // seek in the input file behind the already decoded data
  d->inputFile.at( d->seekPositions[frame] );

  kdDebug() << "(K3bMadDecoder) Seeking to frame " << frame << " with " << frameReservoirProtect << " reservoir frames." << endl;

  // decode some frames ignoring MAD_ERROR_BADDATAPTR errors
  unsigned int i = 1;
  while( i <= frameReservoirProtect ) {
    madStreamBuffer();
    if( mad_frame_decode( d->madFrame, d->madStream ) ) {
      if( MAD_RECOVERABLE( d->madStream->error ) ) {
	if( d->madStream->error == MAD_ERROR_BUFLEN )
	  continue;
	else if( d->madStream->error != MAD_ERROR_BADDATAPTR ) {
	  kdDebug() << "(K3bMadDecoder) Seeking: recoverable mad error (" << mad_stream_errorstr(d->madStream) << ")" << endl;
	  continue;
	}
	else {
	  kdDebug() << "(K3bMadDecoder) Seeking: ignoring (" << mad_stream_errorstr(d->madStream) << ")" << endl;
	}
      }
      else
	return false;
    }

    if( i == frameReservoirProtect )  // synth only the last frame (Rob said so ;)
      mad_synth_frame( d->madSynth, d->madFrame );

    ++i;
  }

  return true;
}


QString K3bMadDecoder::fileType() const
{
  switch( d->firstHeader.layer ) {
  case MAD_LAYER_I:
    return "MPEG1 Layer I";
  case MAD_LAYER_II:
    return "MPEG1 Layer II";
  case MAD_LAYER_III:
    return "MPEG1 Layer III";
  default:
    return "Mp3";
  }
}

QStringList K3bMadDecoder::supportedTechnicalInfos() const
{
  return QStringList::split( ";", 
			     i18n("Channels") + ";" +
			     i18n("Sampling Rate") + ";" +
			     i18n("Bitrate") + ";" +
			     i18n("Layer") + ";" +
			     i18n("Emphasis") + ";" +
			     i18n("Copyright") + ";" +
			     i18n("Original") + ";" +
			     i18n("CRC") );
}


QString K3bMadDecoder::technicalInfo( const QString& name ) const
{
  if( name == i18n("Channels") ) {
    switch( d->firstHeader.mode ) {
    case MAD_MODE_SINGLE_CHANNEL:
      return i18n("Mono");
    case MAD_MODE_DUAL_CHANNEL:
      return i18n("Dual");
    case MAD_MODE_JOINT_STEREO:
      return i18n("Joint Stereo");
    case MAD_MODE_STEREO:
      return i18n("Stereo");
    default:
      return "?";
    }
  }
  else if( name == i18n("Sampling Rate") )
    return i18n("%1 Hz").arg(d->firstHeader.samplerate);
  else if( name == i18n("Bitrate") ) {
    if( d->vbr )
      return i18n("VBR");
    else
      return i18n("%1 bps").arg(d->firstHeader.bitrate);
  }
  else if(  name == i18n("Layer") ){
    switch( d->firstHeader.layer ) {
    case MAD_LAYER_I:
      return "I";
    case MAD_LAYER_II:
      return "II";
    case MAD_LAYER_III:
      return "III";
    default:
      return "?";
    }
  }
  else if( name == i18n("Emphasis") ) {
    switch( d->firstHeader.emphasis ) {
    case MAD_EMPHASIS_NONE:
      return i18n("None");
    case MAD_EMPHASIS_50_15_US:
      return i18n("50/15 ms");
    case MAD_EMPHASIS_CCITT_J_17:
      return i18n("CCITT J.17");
    default:
      return i18n("Unknown");
    }
  }
  else if( name == i18n("Copyright") )
    return ( d->firstHeader.flags & MAD_FLAG_COPYRIGHT ? i18n("Yes") : i18n("No") );
  else if( name == i18n("Original") )
    return ( d->firstHeader.flags & MAD_FLAG_ORIGINAL ? i18n("Yes") : i18n("No") );
  else if( name == i18n("CRC") )
    return ( d->firstHeader.flags & MAD_FLAG_PROTECTION ? i18n("Yes") : i18n("No") );
  else
    return QString::null;
}


K3bMadDecoderFactory::K3bMadDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
  s_instance = new KInstance( "k3bmaddecoder" );
}


K3bMadDecoderFactory::~K3bMadDecoderFactory()
{
}


K3bPlugin* K3bMadDecoderFactory::createPluginObject( QObject* parent, 
							   const char* name,
							   const QStringList& )
{
  return new K3bMadDecoder( parent, name );
}

bool K3bMadDecoderFactory::canDecode( const KURL& url )
{
//   static const QString mime_types[] = {
//     "audio/mp3", "audio/x-mp3", "audio/mpg3", "audio/x-mpg3", "audio/mpeg3", "audio/x-mpeg3",
//     "audio/mp2", "audio/x-mp2", "audio/mpg2", "audio/x-mpg2", "audio/mpeg2", "audio/x-mpeg2",
//     "audio/mp1", "audio/x-mp1", "audio/mpg1", "audio/x-mpg1", "audio/mpeg1", "audio/x-mpeg1",
//     "audio/mpeg", "audio/x-mpeg",
//     QString::null,
//   };

//   QString mimetype = KMimeType::findByFileContent( url.path(), 0 )->name();
//   kdDebug() << "(K3bMadDecoder) mimetype: " << mimetype << endl;

//   for( int i = 0; !mime_types[i].isNull(); ++i )
//     if( mime_types[i] == mimetype )
//       return true;

  // no success with the mimetype
  // since sometimes the mimetype system does not work we try it on our own


  QFile f(url.path());
  if( !f.open(IO_ReadOnly) ) {
    kdDebug() << "(K3bMadDecoder) could not open file " << url.path() << endl;
    return false;
  }

  // there seem to be mp3 files starting with a lot of zeros
  // we try to skip these.
  // there might still be files with more than bufLen zeros...
  const int bufLen = 4096;
  char buf[bufLen];
  if( f.readBlock( buf, bufLen ) < bufLen ) {
    kdDebug() << "(K3bMadDecoder) unable to read " << bufLen << " bytes from " << url.path() << endl;
    f.close();
    return false;
  }
  f.close();

  // skip any 0
  int i = 0;
  while( i < bufLen && buf[i] == '\0' ) i++;
  if( i == bufLen ) {
    kdDebug() << "(K3bMadDecoder) only zeros found in the beginning of " << url.path() << endl;
    return false;
  }



  // now check if the file starts with an id3 tag
  if( i < bufLen-5 && 
      ( buf[i] == 'I' && buf[i+1] == 'D' && buf[i+2] == '3' ) &&
      ( (unsigned short)buf[i+3] < 0xff && (unsigned short)buf[i+4] < 0xff ) ) {
    kdDebug() << "(K3bMadDecoder) found id3 magic: ID3 " 
	      << (unsigned short)buf[i+3] << "." << (unsigned short)buf[i+4] << endl;
    return true;
  }

  // check if we have a RIFF MPEG header
  // libmad seems to be able to decode such files but not to decode these first bytes
  if( ( buf[i] == 'R' && buf[i+1] == 'I' && buf[i+2] == 'F' && buf[i+3] == 'F' &&
	buf[i+8] == 'W' && buf[i+9] == 'A' && buf[i+10] == 'V' && buf[i+11] == 'E' &&
	buf[i+12] == 'f' && buf[i+13] == 'm' &&	buf[i+14] == 't' ) ) {
    kdDebug() << "(K3bMadDecoder) found RIFF, WAVE, and fmt." << endl;
    short m = (short)( buf[i+20] | (buf[i+21]<<8) );
    if( m == 80 ) {
      kdDebug() << "(K3bMadDecoder) found RIFF MPEG magic." << endl;
      return true;
    }
    else if( m == 85 ) {
      kdDebug() << "(K3bMadDecoder) found RIFF MPEG III magic." << endl;
      return true;
    }
    else
      return false;
  }
      


  // here no id3 tag could be found
  // so let libmad try to decode one frame header
  mad_stream stream;;
  mad_header header;
  mad_stream_init( &stream );
  mad_header_init( &header );

  mad_stream_buffer( &stream, (unsigned char*)&buf[i], bufLen-i );
  stream.error = MAD_ERROR_NONE;
  bool success = true;
  if( mad_header_decode( &header, &stream ) ) {
    kdDebug() << "(K3bMadDecoder) could not find mpeg header." << endl;
    success = false;
  }

  mad_header_finish( &header );
  mad_stream_finish( &stream );
  
  return success;
}

#include "k3bmaddecoder.moc"
