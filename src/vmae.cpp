#include <stdio.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

const int INBUF_SIZE = 4096;

void pgmSave(unsigned char *buf, int wrap, int xSize, int ySize,
             char *fileName) {
  FILE *f = fopen(fileName, "wb");
  fprintf(f, "P5\n%d %d\n%d\n", xSize, ySize, 255);
  for (int i = 0; i < ySize; i++)
    fwrite(buf + i * wrap, 1, xSize, f);
  fclose(f);
}

void decode(AVCodecContext *dc, AVFrame *frame, AVPacket *pkt,
            const char *fileName) {
  char buf[1024];
  int ret = avcodec_send_packet(dc, pkt);
  if (ret < 0) {
    fprintf(stderr, "Error sending a packet for decoding.\n");
    exit(1);
  }

  while (ret >= 0) {
    ret = avcodec_receive_frame(dc, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      return;
    else if (ret < 0) {
      fprintf(stderr, "Error during decoding.\n");
      exit(1);
    }

    printf("saving frame %3" PRId64 "\n", dc->frame_num);
    fflush(stdout);

    snprintf(buf, sizeof(buf), "%s-%" PRId64 ".pgm", fileName, dc->frame_num);
    pgmSave(frame->data[0], frame->linesize[0], 480, 320, buf);
  }
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
    exit(0);
  }

  const char *inFileName = argv[1];
  const char *outFileName = argv[2];

  AVPacket *pkt = av_packet_alloc();
  if (!pkt)
    exit(1);

  uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
  // set end of buffer to 0; ensures no overreading of damaged MPEG streq
  memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
  if (!codec) {
    fprintf(stderr, "Codec not found.\n");
    exit(1);
  }

  AVCodecParserContext *parser = av_parser_init(codec->id);
  if (!parser) {
    fprintf(stderr, "Parser not found.\n");
    exit(1);
  }

  AVCodecContext *c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context.\n");
    exit(1);
  }
  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec.\n");
    exit(1);
  }

  /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized here because this information is not
       available in the bitstream. */

  FILE *f = fopen(inFileName, "rb");
  if (!f) {
    fprintf(stderr, "Could not open file: %s.\n", inFileName);
    exit(1);
  }

  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate video frame.\n");
    exit(1);
  }

  size_t dataSize;
  uint8_t *data;
  int ret;
  int eof;

  do {
    // read binary from the input file
    dataSize = fread(inbuf, 1, INBUF_SIZE, f);
    if (ferror(f))
      break;
    eof = !dataSize;

    // use parser to split the data into frames
    data = inbuf;
    while (dataSize > 0 || eof) {
      ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, dataSize,
                             AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (ret < 0) {
        fprintf(stderr, "Error while parsing.\n");
        exit(1);
      }
      data += ret;
      dataSize -= ret;

      if (pkt->size)
        decode(c, frame, pkt, outFileName);
      else if (eof)
        break;
    }
  } while (!eof);

  // flush decoder
  decode(c, frame, NULL, outFileName);

  fclose(f);

  av_parser_close(parser);
  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);

  return 0;
}
