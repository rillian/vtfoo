/* MacOS VideoToolbox command-line tool. */

/* This Source Code Form is subject to the terms of the Mozilla Public
 *  * License, v. 2.0. If a copy of the MPL was not distributed with this
 *   * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
  CMSampleBufferRef buffer = NULL;
  VTDecodeInfoFlags flags;

  /* Initialize the decoder. */
  CMVideoFormatDescriptionCreate(NULL, kCMVideoCodecType_H264, 1280, 720, NULL, &format);
  VTDecompressionSessionCreate(NULL, format, NULL, NULL, &cb, &session);

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
