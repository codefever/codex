
// modified from lasso/codex

//
// CODEX has been the name of a well known utility used in Google to
// dump and print content of binary formats like RecordIO and SSTable.
// In MRML, we implemented RecordIO in mrml_recordio.{hh,cc}, so we
// provide this utility (also named CODEX) to dump content of files
// generated using APIs defined in mrml_recordio.{hh,cc}.
//
// Usage:
/*
  codex --message_name=<a-message-name> \
        --protofile=<a-.proto-file>     \
        <one-or-more-data-files>
*/
// Each file among the <one-or-more-data-files> should be a RecordIO
// file consisting of a sequence of
// {message_size,KeyValuePair_message} pairs, and the value of
// KeyValuePair is an serialized protobuf message <a-message-name>
// defined in <a-.proto-file>.

#include <stdio.h>              // For FILE* fopen and fileno().

#include <string>
#include <vector>

#include "glog/logging.h"
#include "google/gflags.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

using google::protobuf::FileDescriptorProto;
using std::string;
using std::vector;

DEFINE_string(message_name, "", "the message defined in .proto file");
DEFINE_string(protofile, "",
    "the .proto file that defines data records in data file");


//-----------------------------------------------------------------------------
// Parsing given .proto file for Descriptor of the given message (by
// name).  The returned message descriptor can be used with a
// DynamicMessageFactory in order to create prototype message and
// mutable messages.  For example:
/*
  DynamicMessageFactory factory;
  const Message* prototype_msg = factory.GetPrototype(message_descriptor);
  const Message* mutable_msg = prototype_msg->New();
*/
//-----------------------------------------------------------------------------
void GetMessageTypeFromProtoFile(const string& proto_filename,
                                 FileDescriptorProto* file_desc_proto) {
  using google::protobuf::io::FileInputStream;
  using google::protobuf::io::Tokenizer;
  using google::protobuf::compiler::Parser;

  FILE* proto_file = fopen(proto_filename.c_str(), "r");
  {
    if (proto_file == NULL) {
      LOG(FATAL) << "Cannot open .proto file: " << proto_filename;
    }

    FileInputStream proto_input_stream(fileno(proto_file));
    Tokenizer tokenizer(&proto_input_stream, NULL);
    Parser parser;
    if (!parser.Parse(&tokenizer, file_desc_proto)) {
      LOG(FATAL) << "Cannot parse .proto file:" << proto_filename;
    }
  }
  fclose(proto_file);

  // Here we walk around a bug in protocol buffers that
  // |Parser::Parse| does not set name (.proto filename) in
  // file_desc_proto.
  if (!file_desc_proto->has_name()) {
    file_desc_proto->set_name(proto_filename);
  }
}

//-----------------------------------------------------------------------------
// Print contents of a record file with following format:
//
//   { <int record_size> <KeyValuePair> }
//
// where KeyValuePair is a proto message defined in mrml.proto, and
// consists of two string fields: key and value, where key will be
// printed as a text string, and value will be parsed into a proto
// message given as |message_descriptor|.
//-----------------------------------------------------------------------------
void PrintDataFile(const string& data_filename,
                   const FileDescriptorProto& file_desc_proto,
                   const string& message_name) {
  FILE* input_stream = fopen(data_filename.c_str(), "r");
  if (input_stream == NULL) {
    LOG(FATAL) << "Cannot open data file: " << data_filename;
  }

  google::protobuf::DescriptorPool pool;
  const google::protobuf::FileDescriptor* file_desc =
    pool.BuildFile(file_desc_proto);
  if (file_desc == NULL) {
    LOG(FATAL) << "Cannot get file descriptor from file descriptor"
               << file_desc_proto.DebugString();
  }

  const google::protobuf::Descriptor* message_desc =
    file_desc->FindMessageTypeByName(message_name);
  if (message_desc == NULL) {
    LOG(FATAL) << "Cannot get message descriptor of message: " << message_name;
  }

  google::protobuf::DynamicMessageFactory factory;
  const google::protobuf::Message* prototype_msg =
    factory.GetPrototype(message_desc);
  if (prototype_msg == NULL) {
    LOG(FATAL) << "Cannot create prototype message from message descriptor";
  }
  google::protobuf::Message* mutable_msg = prototype_msg->New();
  if (mutable_msg == NULL) {
    LOG(FATAL) << "Failed in prototype_msg->New(); to create mutable message";
  }

  if (!mutable_msg->ParseFromFileDescriptor(fileno(input_stream))) {
    LOG(FATAL) << "Failed to parse data in " << data_filename;
  }
  printf("%s\n", mutable_msg->DebugString().c_str());

  delete mutable_msg;
  fclose(input_stream);
}

int main(int argc, char** argv) {
  FLAGS_logtostderr = true;
  ::google::ParseCommandLineFlags(&argc, &argv, true);

  const string &proto_filename = FLAGS_protofile;
  const string &message_name = FLAGS_message_name;
  vector<string> data_filenames;
  for (int i = 1; i < argc; ++i) {
    data_filenames.push_back(argv[i]);
  }

  FileDescriptorProto file_desc_proto;
  GetMessageTypeFromProtoFile(proto_filename, &file_desc_proto);

  for (int i = 0; i < data_filenames.size(); ++i) {
    PrintDataFile(data_filenames[i], file_desc_proto, message_name);
  }

  return 0;
}
