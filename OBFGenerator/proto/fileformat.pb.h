// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: fileformat.proto

#ifndef PROTOBUF_fileformat_2eproto__INCLUDED
#define PROTOBUF_fileformat_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2004000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2004001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_reflection.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_fileformat_2eproto();
void protobuf_AssignDesc_fileformat_2eproto();
void protobuf_ShutdownFile_fileformat_2eproto();

class Blob;
class BlockHeader;

// ===================================================================

class Blob : public ::google::protobuf::Message {
 public:
  Blob();
  virtual ~Blob();
  
  Blob(const Blob& from);
  
  inline Blob& operator=(const Blob& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const Blob& default_instance();
  
  void Swap(Blob* other);
  
  // implements Message ----------------------------------------------
  
  Blob* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Blob& from);
  void MergeFrom(const Blob& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // optional bytes raw = 1;
  inline bool has_raw() const;
  inline void clear_raw();
  static const int kRawFieldNumber = 1;
  inline const ::std::string& raw() const;
  inline void set_raw(const ::std::string& value);
  inline void set_raw(const char* value);
  inline void set_raw(const void* value, size_t size);
  inline ::std::string* mutable_raw();
  inline ::std::string* release_raw();
  
  // optional int32 raw_size = 2;
  inline bool has_raw_size() const;
  inline void clear_raw_size();
  static const int kRawSizeFieldNumber = 2;
  inline ::google::protobuf::int32 raw_size() const;
  inline void set_raw_size(::google::protobuf::int32 value);
  
  // optional bytes zlib_data = 3;
  inline bool has_zlib_data() const;
  inline void clear_zlib_data();
  static const int kZlibDataFieldNumber = 3;
  inline const ::std::string& zlib_data() const;
  inline void set_zlib_data(const ::std::string& value);
  inline void set_zlib_data(const char* value);
  inline void set_zlib_data(const void* value, size_t size);
  inline ::std::string* mutable_zlib_data();
  inline ::std::string* release_zlib_data();
  
  // optional bytes lzma_data = 4;
  inline bool has_lzma_data() const;
  inline void clear_lzma_data();
  static const int kLzmaDataFieldNumber = 4;
  inline const ::std::string& lzma_data() const;
  inline void set_lzma_data(const ::std::string& value);
  inline void set_lzma_data(const char* value);
  inline void set_lzma_data(const void* value, size_t size);
  inline ::std::string* mutable_lzma_data();
  inline ::std::string* release_lzma_data();
  
  // optional bytes bzip2_data = 5;
  inline bool has_bzip2_data() const;
  inline void clear_bzip2_data();
  static const int kBzip2DataFieldNumber = 5;
  inline const ::std::string& bzip2_data() const;
  inline void set_bzip2_data(const ::std::string& value);
  inline void set_bzip2_data(const char* value);
  inline void set_bzip2_data(const void* value, size_t size);
  inline ::std::string* mutable_bzip2_data();
  inline ::std::string* release_bzip2_data();
  
  // @@protoc_insertion_point(class_scope:Blob)
 private:
  inline void set_has_raw();
  inline void clear_has_raw();
  inline void set_has_raw_size();
  inline void clear_has_raw_size();
  inline void set_has_zlib_data();
  inline void clear_has_zlib_data();
  inline void set_has_lzma_data();
  inline void clear_has_lzma_data();
  inline void set_has_bzip2_data();
  inline void clear_has_bzip2_data();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* raw_;
  ::std::string* zlib_data_;
  ::std::string* lzma_data_;
  ::std::string* bzip2_data_;
  ::google::protobuf::int32 raw_size_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(5 + 31) / 32];
  
  friend void  protobuf_AddDesc_fileformat_2eproto();
  friend void protobuf_AssignDesc_fileformat_2eproto();
  friend void protobuf_ShutdownFile_fileformat_2eproto();
  
  void InitAsDefaultInstance();
  static Blob* default_instance_;
};
// -------------------------------------------------------------------

class BlockHeader : public ::google::protobuf::Message {
 public:
  BlockHeader();
  virtual ~BlockHeader();
  
  BlockHeader(const BlockHeader& from);
  
  inline BlockHeader& operator=(const BlockHeader& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const BlockHeader& default_instance();
  
  void Swap(BlockHeader* other);
  
  // implements Message ----------------------------------------------
  
  BlockHeader* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const BlockHeader& from);
  void MergeFrom(const BlockHeader& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // required string type = 1;
  inline bool has_type() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 1;
  inline const ::std::string& type() const;
  inline void set_type(const ::std::string& value);
  inline void set_type(const char* value);
  inline void set_type(const char* value, size_t size);
  inline ::std::string* mutable_type();
  inline ::std::string* release_type();
  
  // optional bytes indexdata = 2;
  inline bool has_indexdata() const;
  inline void clear_indexdata();
  static const int kIndexdataFieldNumber = 2;
  inline const ::std::string& indexdata() const;
  inline void set_indexdata(const ::std::string& value);
  inline void set_indexdata(const char* value);
  inline void set_indexdata(const void* value, size_t size);
  inline ::std::string* mutable_indexdata();
  inline ::std::string* release_indexdata();
  
  // required int32 datasize = 3;
  inline bool has_datasize() const;
  inline void clear_datasize();
  static const int kDatasizeFieldNumber = 3;
  inline ::google::protobuf::int32 datasize() const;
  inline void set_datasize(::google::protobuf::int32 value);
  
  // @@protoc_insertion_point(class_scope:BlockHeader)
 private:
  inline void set_has_type();
  inline void clear_has_type();
  inline void set_has_indexdata();
  inline void clear_has_indexdata();
  inline void set_has_datasize();
  inline void clear_has_datasize();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* type_;
  ::std::string* indexdata_;
  ::google::protobuf::int32 datasize_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(3 + 31) / 32];
  
  friend void  protobuf_AddDesc_fileformat_2eproto();
  friend void protobuf_AssignDesc_fileformat_2eproto();
  friend void protobuf_ShutdownFile_fileformat_2eproto();
  
  void InitAsDefaultInstance();
  static BlockHeader* default_instance_;
};
// ===================================================================


// ===================================================================

// Blob

// optional bytes raw = 1;
inline bool Blob::has_raw() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Blob::set_has_raw() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Blob::clear_has_raw() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Blob::clear_raw() {
  if (raw_ != &::google::protobuf::internal::kEmptyString) {
    raw_->clear();
  }
  clear_has_raw();
}
inline const ::std::string& Blob::raw() const {
  return *raw_;
}
inline void Blob::set_raw(const ::std::string& value) {
  set_has_raw();
  if (raw_ == &::google::protobuf::internal::kEmptyString) {
    raw_ = new ::std::string;
  }
  raw_->assign(value);
}
inline void Blob::set_raw(const char* value) {
  set_has_raw();
  if (raw_ == &::google::protobuf::internal::kEmptyString) {
    raw_ = new ::std::string;
  }
  raw_->assign(value);
}
inline void Blob::set_raw(const void* value, size_t size) {
  set_has_raw();
  if (raw_ == &::google::protobuf::internal::kEmptyString) {
    raw_ = new ::std::string;
  }
  raw_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Blob::mutable_raw() {
  set_has_raw();
  if (raw_ == &::google::protobuf::internal::kEmptyString) {
    raw_ = new ::std::string;
  }
  return raw_;
}
inline ::std::string* Blob::release_raw() {
  clear_has_raw();
  if (raw_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = raw_;
    raw_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional int32 raw_size = 2;
inline bool Blob::has_raw_size() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Blob::set_has_raw_size() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Blob::clear_has_raw_size() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Blob::clear_raw_size() {
  raw_size_ = 0;
  clear_has_raw_size();
}
inline ::google::protobuf::int32 Blob::raw_size() const {
  return raw_size_;
}
inline void Blob::set_raw_size(::google::protobuf::int32 value) {
  set_has_raw_size();
  raw_size_ = value;
}

// optional bytes zlib_data = 3;
inline bool Blob::has_zlib_data() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Blob::set_has_zlib_data() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Blob::clear_has_zlib_data() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Blob::clear_zlib_data() {
  if (zlib_data_ != &::google::protobuf::internal::kEmptyString) {
    zlib_data_->clear();
  }
  clear_has_zlib_data();
}
inline const ::std::string& Blob::zlib_data() const {
  return *zlib_data_;
}
inline void Blob::set_zlib_data(const ::std::string& value) {
  set_has_zlib_data();
  if (zlib_data_ == &::google::protobuf::internal::kEmptyString) {
    zlib_data_ = new ::std::string;
  }
  zlib_data_->assign(value);
}
inline void Blob::set_zlib_data(const char* value) {
  set_has_zlib_data();
  if (zlib_data_ == &::google::protobuf::internal::kEmptyString) {
    zlib_data_ = new ::std::string;
  }
  zlib_data_->assign(value);
}
inline void Blob::set_zlib_data(const void* value, size_t size) {
  set_has_zlib_data();
  if (zlib_data_ == &::google::protobuf::internal::kEmptyString) {
    zlib_data_ = new ::std::string;
  }
  zlib_data_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Blob::mutable_zlib_data() {
  set_has_zlib_data();
  if (zlib_data_ == &::google::protobuf::internal::kEmptyString) {
    zlib_data_ = new ::std::string;
  }
  return zlib_data_;
}
inline ::std::string* Blob::release_zlib_data() {
  clear_has_zlib_data();
  if (zlib_data_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = zlib_data_;
    zlib_data_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional bytes lzma_data = 4;
inline bool Blob::has_lzma_data() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void Blob::set_has_lzma_data() {
  _has_bits_[0] |= 0x00000008u;
}
inline void Blob::clear_has_lzma_data() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void Blob::clear_lzma_data() {
  if (lzma_data_ != &::google::protobuf::internal::kEmptyString) {
    lzma_data_->clear();
  }
  clear_has_lzma_data();
}
inline const ::std::string& Blob::lzma_data() const {
  return *lzma_data_;
}
inline void Blob::set_lzma_data(const ::std::string& value) {
  set_has_lzma_data();
  if (lzma_data_ == &::google::protobuf::internal::kEmptyString) {
    lzma_data_ = new ::std::string;
  }
  lzma_data_->assign(value);
}
inline void Blob::set_lzma_data(const char* value) {
  set_has_lzma_data();
  if (lzma_data_ == &::google::protobuf::internal::kEmptyString) {
    lzma_data_ = new ::std::string;
  }
  lzma_data_->assign(value);
}
inline void Blob::set_lzma_data(const void* value, size_t size) {
  set_has_lzma_data();
  if (lzma_data_ == &::google::protobuf::internal::kEmptyString) {
    lzma_data_ = new ::std::string;
  }
  lzma_data_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Blob::mutable_lzma_data() {
  set_has_lzma_data();
  if (lzma_data_ == &::google::protobuf::internal::kEmptyString) {
    lzma_data_ = new ::std::string;
  }
  return lzma_data_;
}
inline ::std::string* Blob::release_lzma_data() {
  clear_has_lzma_data();
  if (lzma_data_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = lzma_data_;
    lzma_data_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional bytes bzip2_data = 5;
inline bool Blob::has_bzip2_data() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void Blob::set_has_bzip2_data() {
  _has_bits_[0] |= 0x00000010u;
}
inline void Blob::clear_has_bzip2_data() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void Blob::clear_bzip2_data() {
  if (bzip2_data_ != &::google::protobuf::internal::kEmptyString) {
    bzip2_data_->clear();
  }
  clear_has_bzip2_data();
}
inline const ::std::string& Blob::bzip2_data() const {
  return *bzip2_data_;
}
inline void Blob::set_bzip2_data(const ::std::string& value) {
  set_has_bzip2_data();
  if (bzip2_data_ == &::google::protobuf::internal::kEmptyString) {
    bzip2_data_ = new ::std::string;
  }
  bzip2_data_->assign(value);
}
inline void Blob::set_bzip2_data(const char* value) {
  set_has_bzip2_data();
  if (bzip2_data_ == &::google::protobuf::internal::kEmptyString) {
    bzip2_data_ = new ::std::string;
  }
  bzip2_data_->assign(value);
}
inline void Blob::set_bzip2_data(const void* value, size_t size) {
  set_has_bzip2_data();
  if (bzip2_data_ == &::google::protobuf::internal::kEmptyString) {
    bzip2_data_ = new ::std::string;
  }
  bzip2_data_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Blob::mutable_bzip2_data() {
  set_has_bzip2_data();
  if (bzip2_data_ == &::google::protobuf::internal::kEmptyString) {
    bzip2_data_ = new ::std::string;
  }
  return bzip2_data_;
}
inline ::std::string* Blob::release_bzip2_data() {
  clear_has_bzip2_data();
  if (bzip2_data_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = bzip2_data_;
    bzip2_data_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// -------------------------------------------------------------------

// BlockHeader

// required string type = 1;
inline bool BlockHeader::has_type() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void BlockHeader::set_has_type() {
  _has_bits_[0] |= 0x00000001u;
}
inline void BlockHeader::clear_has_type() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void BlockHeader::clear_type() {
  if (type_ != &::google::protobuf::internal::kEmptyString) {
    type_->clear();
  }
  clear_has_type();
}
inline const ::std::string& BlockHeader::type() const {
  return *type_;
}
inline void BlockHeader::set_type(const ::std::string& value) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::kEmptyString) {
    type_ = new ::std::string;
  }
  type_->assign(value);
}
inline void BlockHeader::set_type(const char* value) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::kEmptyString) {
    type_ = new ::std::string;
  }
  type_->assign(value);
}
inline void BlockHeader::set_type(const char* value, size_t size) {
  set_has_type();
  if (type_ == &::google::protobuf::internal::kEmptyString) {
    type_ = new ::std::string;
  }
  type_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* BlockHeader::mutable_type() {
  set_has_type();
  if (type_ == &::google::protobuf::internal::kEmptyString) {
    type_ = new ::std::string;
  }
  return type_;
}
inline ::std::string* BlockHeader::release_type() {
  clear_has_type();
  if (type_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = type_;
    type_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional bytes indexdata = 2;
inline bool BlockHeader::has_indexdata() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void BlockHeader::set_has_indexdata() {
  _has_bits_[0] |= 0x00000002u;
}
inline void BlockHeader::clear_has_indexdata() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void BlockHeader::clear_indexdata() {
  if (indexdata_ != &::google::protobuf::internal::kEmptyString) {
    indexdata_->clear();
  }
  clear_has_indexdata();
}
inline const ::std::string& BlockHeader::indexdata() const {
  return *indexdata_;
}
inline void BlockHeader::set_indexdata(const ::std::string& value) {
  set_has_indexdata();
  if (indexdata_ == &::google::protobuf::internal::kEmptyString) {
    indexdata_ = new ::std::string;
  }
  indexdata_->assign(value);
}
inline void BlockHeader::set_indexdata(const char* value) {
  set_has_indexdata();
  if (indexdata_ == &::google::protobuf::internal::kEmptyString) {
    indexdata_ = new ::std::string;
  }
  indexdata_->assign(value);
}
inline void BlockHeader::set_indexdata(const void* value, size_t size) {
  set_has_indexdata();
  if (indexdata_ == &::google::protobuf::internal::kEmptyString) {
    indexdata_ = new ::std::string;
  }
  indexdata_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* BlockHeader::mutable_indexdata() {
  set_has_indexdata();
  if (indexdata_ == &::google::protobuf::internal::kEmptyString) {
    indexdata_ = new ::std::string;
  }
  return indexdata_;
}
inline ::std::string* BlockHeader::release_indexdata() {
  clear_has_indexdata();
  if (indexdata_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = indexdata_;
    indexdata_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// required int32 datasize = 3;
inline bool BlockHeader::has_datasize() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void BlockHeader::set_has_datasize() {
  _has_bits_[0] |= 0x00000004u;
}
inline void BlockHeader::clear_has_datasize() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void BlockHeader::clear_datasize() {
  datasize_ = 0;
  clear_has_datasize();
}
inline ::google::protobuf::int32 BlockHeader::datasize() const {
  return datasize_;
}
inline void BlockHeader::set_datasize(::google::protobuf::int32 value) {
  set_has_datasize();
  datasize_ = value;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_fileformat_2eproto__INCLUDED
