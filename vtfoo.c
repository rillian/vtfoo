/* MacOS VideoToolbox command-line tool. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>

#include <VideoToolbox/VideoToolbox.h>

/* Decoded data callback. VideoToolbox will call us with successive
 * (decode or presentation order, depending on flags) frames of data. */
void decode_cb(
    void* decompressionOutputRefCon,
    void* sourceFrameRefCon,
    OSStatus status,
    VTDecodeInfoFlags info,
    CVImageBufferRef imageBuffer,
    CMTime presentationTimeStamp,
    CMTime presentationDuration)
{
  if (status != noErr || !imageBuffer) {
    fprintf(stderr, "Error %d decoding frame!\n", status);
    return;
  }

  fprintf(stdout, "Got frame data.\n");
}
          
/* Smoke test decoding. */
int decode(void) 
{
  VTDecompressionSessionRef session;
  VTDecompressionOutputCallbackRecord cb = { decode_cb, NULL };
  CMVideoFormatDescriptionRef format;
  CMBlockBufferRef block = NULL;
  CMSampleBufferRef buffer = NULL;
  VTDecodeInfoFlags flags;
  OSStatus status;

  /* Initialize the decoder. */
  CMVideoFormatDescriptionCreate(NULL, kCMVideoCodecType_H264, 1280, 720, NULL, &format);
  VTDecompressionSessionCreate(NULL, format, NULL, NULL, &cb, &session);

  /* Wrap our demuxed frame data for passing to the decoder. */
  { FILE *in = fopen("mdat", "rb");
    const int mdat_size = 1000000;
    uint8_t *mdat = malloc(mdat_size);
    size_t read = fread(mdat, 1, mdat_size, in);
    fclose(in);
    status = CMBlockBufferCreateWithMemoryBlock(NULL, mdat, read, NULL, NULL, 0, read, false, &block);
    if (status != noErr) return status;
    status = CMSampleBufferCreate(NULL, block, TRUE, 0, 0, format, 1, 1, NULL, 0, NULL, &buffer);
    if (status != noErr) return status;
  }

  /* Loop over compressed data; our callback will be called with
   * each decoded frame buffer. Passed flags make this asynchronous. */
  VTDecompressionSessionDecodeFrame(session, buffer,
      kVTDecodeFrame_EnableAsynchronousDecompression |
      kVTDecodeFrame_EnableTemporalProcessing,
      &buffer, &flags);

  /* Flush in-process frames. */
  VTDecompressionSessionFinishDelayedFrames(session);
  /* Block until our callback has been called with the last frame. */
  VTDecompressionSessionWaitForAsynchronousFrames(session);

  /* Clean up. */
  VTDecompressionSessionInvalidate(session);
  CFRelease(session);
  CFRelease(format);

  return 0;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  
  ret = decode();

  return ret;
}
