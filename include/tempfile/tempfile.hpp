/// Copyright (c) 2024 David Parrini
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.

#ifndef TEMPFILE_TEMPFILE_HPP
#define TEMPFILE_TEMPFILE_HPP

// This module handles the creation of temporary files and directories and their cleanup.
// It is based on the RAII idiom and is inspired in Python's tempfile module implementation.

#include <string>
#include <stdexcept>


// test if its MSVC compiler
#if defined(_MSC_VER)
#if _MSVC_LANG < 201703L
#undef TEMPFILE_HAS_FILESYSTEM
#else
#define TEMPFILE_HAS_FILESYSTEM
#endif
#elif __cplusplus >= 201703L
#define TEMPFILE_HAS_FILESYSTEM
#else
#under TEMPFILE_HAS_FILESYSTEM
#endif

#ifdef TEMPFILE_HAS_FILESYSTEM
#include <filesystem>
#endif


namespace tempfile
{
const std::string default_prefix("tmp");

#ifdef TEMPFILE_HAS_FILESYSTEM
typedef std::filesystem::path path_t;
#else
typedef std::string path_t;
#endif


struct directory
{
  explicit directory(std::string prefix = default_prefix);
  ~directory();
  [[nodiscard]] path_t path() const { return _path; };

  bool create();
  bool remove();

  [[nodiscard]] bool good() const { return _good; };

private:
  bool _good;
  std::string const _prefix;
  path_t _path;
};


struct scoped_directory : public directory
{
  explicit scoped_directory(std::string prefix = default_prefix);
  ~scoped_directory();
};


struct file
{
  explicit file(std::string prefix = default_prefix);
  ~file();
  [[nodiscard]] path_t path() const { return _path; };

  bool remove();

  [[nodiscard]] bool good() const { return _good; };
private:
  bool _good;
  std::string _prefix;
  path_t _path;
};

struct scoped_file : public file
{
  explicit scoped_file(std::string prefix = default_prefix);
  ~scoped_file();
};

}

#endif //TEMPFILE_TEMPFILE_HPP
