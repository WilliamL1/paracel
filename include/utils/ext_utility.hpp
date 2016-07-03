/**
 * Copyright (c) 2014, Douban Inc. 
 *   All rights reserved. 
 *
 * Distributed under the BSD License. Check out the LICENSE file for full text.
 *
 * Paracel - A distributed optimization framework with parameter server.
 *
 * Downloading
 *   git clone https://github.com/douban/paracel.git
 *
 * Authors: Hong Wu <xunzhangthu@gmail.com>
 *
 */

#ifndef FILE_56636034_fc34_4f9e_4343_4111ca3b127d_HPP
#define FILE_56636034_fc34_4f9e_4343_4111ca3b127d_HPP

#include <sys/stat.h>
#include <glob.h>

#include <map>
#include <ctime>
#include <vector>
#include <numeric>
#include <utility>
#include <fstream>
#include <unordered_map>

#include <boost/algorithm/string/regex.hpp>

#include "paracel_types.hpp"

namespace paracel {

typedef paracel::list_type< paracel::str_type > string_lst;

string_lst str_split(const paracel::str_type & str, 
                     const char sep) {
  string_lst result;
  size_t st = 0, en = 0;
  while(1) {
    en = str.find(sep, st);
    auto s = str.substr(st, en - st);
    if(s.size()) result.push_back(std::move(s));
    if(en == paracel::str_type::npos) break;
    st = en + 1;
  }
  return result;
}

string_lst str_split(const paracel::str_type & str, 
                     const paracel::str_type & seps) {
  string_lst result;
  size_t st = 0, en = 0;
  while(1) {
    en = str.find_first_of(seps, st);
    auto s = str.substr(st, en - st);
    if(s.size()) result.push_back(std::move(s));
    if(en == paracel::str_type::npos) break;
    st = en + 1;
  }
  return result;
}

template <class T>
string_lst str_split_by_word(T && str, 
                             const paracel::str_type & seps) {
  string_lst result;
  boost::algorithm::split_regex(result, std::forward<T>(str), boost::regex(seps));
  if(result.back() == "") result.pop_back();
  return result;
}

paracel::str_type str_join(const string_lst & strlst, 
                           const paracel::str_type & seps) {
  paracel::str_type r;
  for(size_t i = 0; i < strlst.size() - 1; ++i) {
    r.append(strlst[i]);
    r.append(seps);
  }
  r.append(strlst.back());
  return r;
}

bool startswith(const paracel::str_type & str,
                const paracel::str_type & key) {
  return str.find(key) == 0;
}

bool endswith(const paracel::str_type & str,
              const std::string & key) {
  if(str.size() < key.size()) return false;
  return str.rfind(key) == (str.size() - key.size());
}

bool isfile(const paracel::str_type & f) {
  struct stat st;
  int r = stat(f.c_str(), &st);
  if(r != 0) return false;
  if(S_ISREG(st.st_mode)) return true;
  return false;
}

bool isdir(const paracel::str_type & d) {
  struct stat st;
  int r = stat(d.c_str(), &st);
  if(r != 0) return false;
  if(S_ISDIR(st.st_mode)) return true;
  return false;
}

paracel::list_type<paracel::str_type> 
paracel_glob(const paracel::str_type & pattern) {
  glob_t glob_res;
  paracel::list_type<paracel::str_type> lst;
  glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_res);
  for(size_t i = 0; i < glob_res.gl_pathc; ++i) {
    lst.push_back(std::string(glob_res.gl_pathv[i]));
  }
  globfree(&glob_res);
  return lst;
}

// expand a dictory name recursively
paracel::list_type<paracel::str_type> 
expand_dir_rec(const paracel::str_type & dname) {
 paracel::list_type<paracel::str_type> fl;
 auto dname_new = dname;
 if(!endswith(dname, "/")) {
   dname_new = dname + "/";
 }
 auto lst = paracel_glob(dname_new + "*");
 for(auto & name : lst) {
   if(isfile(name)) {
     fl.push_back(name);
   } else {
     auto tmp_lst = expand_dir_rec(name);
     fl.insert(fl.end(), tmp_lst.begin(), tmp_lst.end());
   }
 }
 return fl;
}

// fname can be 'demo.txt' or 'demo_dir' or 'demo_dir/*.csv'
paracel::list_type<paracel::str_type> 
expand(const paracel::str_type & fname) {
  paracel::list_type<paracel::str_type> fl;
  if(isfile(fname)) {
    fl.push_back(fname);
    return fl;
  } else if(isdir(fname)) {
    return expand_dir_rec(fname); 
  } else {
    // expect a reg exp: '/home/xxx/*.csv'
    return paracel_glob(fname);
  } 
}

// fname_lst can be ['demo.txt', 'demo_dir', 'demo_dir/*.txt', ...]
paracel::list_type<paracel::str_type> 
expand(const paracel::list_type<paracel::str_type> & fname_lst) {
  paracel::list_type<paracel::str_type> fl;
  for(auto & name : fname_lst) {
    auto tmp_lst = expand(name);
    fl.insert(fl.end(), tmp_lst.begin(), tmp_lst.end());
  }
  return fl;
}

double dot_product(const std::vector<double> & a, 
                   const std::vector<double> & b) {
  return std::inner_product(a.begin(), a.end(), b.begin(), 0.);
}

paracel::str_type todir(const paracel::str_type & f) {
  if(endswith(f, "/")) return f;
  return f + "/";
}

std::string add_folder_suffix_with_date(const std::string & folder) {
  time_t t = time(0);
  tm *now = localtime(&t);
  std::string suffix = std::to_string(now->tm_year + 1900) 
      + std::to_string(now->tm_mon + 1) 
      + std::to_string(now->tm_mday);
  std::string new_folder;
  if(endswith(folder, "/")) {
    new_folder = std::string(folder.begin(), folder.end() - 1) + suffix + "/";
  } else {
    new_folder = folder + suffix + "/";
  }
  return new_folder;
}

void file_replace(const std::string & src_file,
                  const std::string & dst_file,
                  const std::unordered_map<std::string, std::string> & rep_map) {
  auto replace_all = [] (std::string l,
                         const std::string & before,
                         const std::string & after) {
    size_t start_indx = 0;
    while((start_indx = l.find(before, start_indx)) != std::string::npos) {
      l.replace(start_indx, before.size(), after);
      start_indx += after.size();
    }
    return l;
  };
  std::ifstream fin(src_file);
  std::ofstream fout(dst_file);
  std::string line;
  while(std::getline(fin, line)) {
    if(startswith(line, "#")) continue;
    for(auto & kv : rep_map) {
      line = replace_all(line, kv.first, kv.second);
    }
    fout << line << std::endl;
  }
  fin.close();
  fout.close();
}

} // namespace paracel
#endif
