#include <unistd.h>

#include "glog/logging.h"
#include "google/gflags.h"
#include "google/protobuf/text_format.h"

#include "mock.pb.h"

DEFINE_string(output, "", "");

int main(int argc, char *argv[]) {
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  const char *kMockData =
      "item {"
      "  name: \"Kao\""
      "  num: 1"
      "  num: 3"
      "  num: 5"
      "  num: 7"
      "}"
      "item {"
      "  name: \"Yooo\""
      "  num: 2"
      "  num: 8"
      "}"
      "item {"
      "  name: \"HiHi\""
      "  num: 33"
      "  num: 99"
      "  num: 100"
      "}";

  Data data;
  CHECK(::google::protobuf::TextFormat::ParseFromString(kMockData, &data))
      << "parse from text failed";

  FILE *fp = fopen(FLAGS_output.c_str(), "w");
  CHECK(fp != NULL) << "fopen failed";
  CHECK(data.SerializeToFileDescriptor(fileno(fp)));
  fclose(fp);
  return 0;
}
