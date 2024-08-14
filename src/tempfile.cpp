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

#include <tempfile/tempfile.hpp>

#include <cstdlib>
#include <mutex>
#include <utility>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else

#endif


#ifndef TEMPFILE_HAS_FILESYSTEM
// unsupported compiler/standard. should not compile.
#error "Unsupported compiler/standard. std::filesystem is required."
#endif


static std::mutex mutex;


#ifdef _WIN32
static const std::string path_separator{'\\'};
#else
static const std::string path_separator{'/'};
#endif

#ifdef _WIN32
static constexpr const size_t max_file_name_length = 255;
#else
static constexpr const int max_file_name_length = 255;
#endif

#ifdef _WIN32
static constexpr const size_t max_path_length = MAX_PATH;
#else
static constexpr const int max_path_length = 4096;
#endif


bool make_directory(tempfile::path_t const & path)
{
  return std::filesystem::create_directory(path);
}

[[nodiscard]] bool directory_exists(tempfile::path_t const & path)
{
  return std::filesystem::is_directory(path);
}

[[nodiscard]] bool file_exists(tempfile::path_t const & path)
{
  return std::filesystem::exists(path);
}

std::vector<tempfile::path_t> get_files_in_directory(tempfile::path_t const & path)
{
  std::vector<tempfile::path_t> files;
  for (auto const & entry : std::filesystem::directory_iterator(path))
  {
    if (entry.is_regular_file())
    {
      files.push_back(entry.path());
    }
  }
  return files;
}


bool remove_file(tempfile::path_t const & path)
{
  return std::filesystem::remove(path);
}

bool remove_directory(tempfile::path_t const & path)
{
  return std::filesystem::remove_all(path);
}


[[nodiscard]] int randrange(int _min, int _max)
{
  return _min + (std::rand() % (_max - _min));
}

[[nodiscard]] std::string random_name()
{
  static auto characters = "abcdefghijklmnopqrstuvwxyz0123456789_";
  auto const k = 8;
  std::string name;
  name.resize(k);
  for (auto i = 0; i < k; ++i)
  {
    name[i] = characters[randrange(0, 36)];
  }
  return name;
}


std::vector<tempfile::path_t> paths_to_try()
{
  std::vector<tempfile::path_t> paths;
  // get paths from environment variable
  for (auto const & var : {"TEMP", "TMP", "TMPDIR"})
  {
    auto path = std::getenv(var);
    if (path != nullptr)
    {
      paths.emplace_back(path);
    }
  }

  // get the system path
#ifdef _WIN32
  auto root_path = std::getenv("SYSTEMROOT");
  if (root_path != nullptr)
  {
    paths.emplace_back(std::string(root_path) + "\\Temp");
  }
  auto userdata_path = std::getenv("USERPROFILE");
  if (userdata_path != nullptr)
  {
    paths.emplace_back(std::string(userdata_path) + "\\AppData\\Local\\Temp");
  }
  for (auto const & path_to_try : {"c:\\temp", "c:\\tmp", })
  {
    paths.emplace_back(path_to_try);
  }

#else
  for(auto const & paths_to_try : {"/tmp", "/var/tmp", "/usr/tmp"})
  {
    paths.push_back(path);
  }
#endif

  // get the current directory as last resort via cwd
#ifdef _WIN32
  auto cwdpath = std::getenv("CD");
  if (cwdpath != nullptr)
  {
    paths.emplace_back(cwdpath);
  }

#else
  auto cwdpath = std::getenv("PWD");
  if (cwdpath != nullptr)
  {
    paths.push_back(cwdpath);
  }

#endif
  return paths;
}


tempfile::directory::directory(std::string  prefix)
  : _good(false), _prefix(std::move(prefix))
{
}

tempfile::directory::~directory()
{
  remove();
}


bool tempfile::directory::create()
{
  std::scoped_lock lock(mutex);

  if (!_path.empty() && _good)
  {
    // TODO: use a specific error message for this.
    return false;
  }
  // get the paths to try
  auto paths = paths_to_try();

  // try to create a directory
  for (auto const & path : paths)
  {
    for (auto itry = 0; itry < 100; ++itry)
    {
      // create a random string
      auto random_string = random_name();

      // check if path is long enough
      if (path.string().length() + path_separator.length() + _prefix.length() + random_string.length() + 1 > max_path_length)
      {
        continue;
      }

      // create a temporary directory
      auto path_to_try = path;
      path_to_try += path_separator;
      path_to_try += _prefix;
      path_to_try += random_string;

      if (directory_exists(path_to_try))
      {
        continue;
      }

      if (make_directory(path_to_try))
      {
        _path = path_to_try;
        _good = true;
        return true;
      }
    }
  }

  // failed to create a directory
  return false;
}

bool tempfile::directory::remove()
{
  std::scoped_lock lock(mutex);
  if (_good && directory_exists(_path))
  {
    for (auto const & file_path : get_files_in_directory(_path))
    {
      remove_file(file_path);
    }
    remove_directory(_path);
    return true;
  }
  return false;
}


tempfile::scoped_directory::scoped_directory(std::string prefix)
  : directory(std::move(prefix))
{
  create();
}

tempfile::scoped_directory::~scoped_directory()
{
  remove();
}


tempfile::file::file(std::string prefix)
  : _good(false), _prefix(std::move(prefix))
{
}

tempfile::file::~file()
{
  std::scoped_lock lock(mutex);
  if (_good && file_exists(_path))
  {
    remove_file(_path);
  }
}

bool tempfile::file::remove()
{
  if (_good && file_exists(_path))
  {
    remove_file(_path);
    return true;
  }
  return false;
}

tempfile::scoped_file::scoped_file(std::string prefix)
  : file(std::move(prefix))
{
}

tempfile::scoped_file::~scoped_file()
{
  remove();
}


