#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "config.h"
#include "init.h"
#include "create.h"
#include "dbstore.h"
#include "cdobs.h"
#include "dberror.h"
#include "utils.h"
#include "help.h"

using namespace std;

// Enums of all named args to
// cdobs, like --db for exampel
enum NamedArg {
  DB = 1, HELP = 2
};

// "" becaue the NamedArg enum starts from 
// 1
vector<pair<string, int>> named_arg_list { make_pair("", 0), make_pair("db", 1), make_pair("help", 0) };


void Help () {
  cout << kHelpCdobs << endl;
}

void InvalidSyntax (const string &err) {
  cout << kErrInvalidSyntax << err << endl;
  cout << "Try 'cdobs --help' for more information." << endl;
}

void InvalidSyntax () {
  InvalidSyntax("");
}

int cmdInit (int argc, char **argv, const string &db_file) {
  if (argc > 1) {
    InvalidSyntax();
    return 1;
  }
  string err_msg;
  int rc = SetupDatabase(db_file, err_msg);
  if (rc) {
    cout << "ERROR: " << kErrInitFailed
    << err_msg << endl;
    return 1;
  }
  return 0;
}


int cmdCreateBucket (Cdobs *const cdobs, const string &bucket_name) {
  int ret_value = 0;
  string err_msg;
  int rc_cb = cdobs->CreateBucket(bucket_name, err_msg);
  if (rc_cb) {
    ret_value = 1;    
    cerr << "ERROR: " << kErrCreateBucketFailed
    << err_msg << endl;
  }   

  return ret_value;
}

int cmdListBuckets (Cdobs *const cdobs) {
  vector<Bucket> buckets;
  string err_msg;
  int rc = cdobs->ListBuckets(buckets, err_msg);
  if (rc) {
    cout << "ERROR: " << err_msg << endl;
    return 1;
  }
  if (buckets.empty()) {
    cout << "No buckets in storage" << endl;
  }
  else {
    cout << setw(5) << "ID" << " "
    << setw(20) << "CREATED "
    << setw(8) << "CNT "
    << "  BUCKET NAME " << endl;
    for (auto b = buckets.begin(); b != buckets.end(); ++b) {
      cout << setw(5) << b->id << " "
      << setw(20) << put_time(b->created, TIME_FORMAT)
      << setw(8) << b->object_count << " "
      << "  " << b->name << endl;
    }   
  }
  return 0;
}

int cmdDeleteBucket (Cdobs *const cdobs, const string &bucket_name) {
  int ret_value = 0;
  string err_msg;
  int rc_cb = cdobs->DeleteBucket(bucket_name, err_msg);
  if (rc_cb) {
    ret_value = 1;    
    cerr << "ERROR: " << kErrDeleteBucketFailed
    << err_msg << endl;
  }   

  return ret_value;
}

int cmdEmptyBucket (Cdobs *const cdobs, const string &bucket_name) {
  int ret_value = 0;
  string err_msg;
  int rc_eb = cdobs->EmptyBucket(bucket_name, err_msg);
  if (rc_eb) {
    ret_value = 1;
    cerr << "ERROR: " << kErrEmptyBucketFailed
    << err_msg << endl;
  }

  return ret_value;
}

int cmdBucket (Cdobs *const cdobs, int &argc, char **&argv) {
  dout << "In cmdBucket" << endl;
  if (argc < 2) {
    return cmdListBuckets(cdobs);
  }

  argc--; argv++;
  string arg1(argv[0]), bucket_name; int rc_op;
  if (argc < 2) {
    InvalidSyntax("Missing OPERATION");
    return 1;
  }
  else {
    bucket_name = argv[1];
  }
  if (arg1 == "create") {
    rc_op = cmdCreateBucket(cdobs, bucket_name);
  }
  else if (arg1 == "delete") {
    rc_op = cmdDeleteBucket(cdobs, bucket_name);
  }
  else if (arg1 == "empty") {
    rc_op = cmdEmptyBucket(cdobs, bucket_name);
  }
  else {
    InvalidSyntax(arg1);
    return 1;
  }
  return rc_op;
}

int cmdPutObject(Cdobs *const cdobs, string &bucket_name,
                string &object_name, string &file_name) {
  if (bucket_name == "" || object_name == ""
      || file_name == "") {
    InvalidSyntax();
    return 1;
  }

  string err_msg;
  int rc = cdobs->PutObject(file_name.c_str(), object_name,
                            bucket_name, err_msg);
  if (rc < 0) {
    cout << "ERROR: " << kErrPutObjectFailed
    << err_msg << endl;
    return 1;
  }
  return 0;
}

int cmdListObjects(Cdobs *cdobs, string &bucket_name) {
  if (bucket_name.empty()) {
    InvalidSyntax("-b BUCKET_NAME required");
    return 1;
  }
  vector<Object> objects;
  string err_msg;
  int rc_list = cdobs->ListObjects(bucket_name, objects, err_msg);
  if (!rc_list) {
    if (!objects.empty()) {
      Object obj;
      cout << "Objects in bucket: " << bucket_name << endl;
      cout << setw(5) << "ID" << " "
      << setw(20) << "CREATED "
      << setw(12) << "SIZE "
      << "  NAME" << endl;
      for (auto it = objects.begin(); it != objects.end(); ++it) {
        obj = *it;
        cout << setw(5) << obj.id << " "
        << setw(20) << put_time(obj.created, TIME_FORMAT)
        << setw(12) << obj.size << " "
        << "  " << obj.name << endl;
      }
    }
    else {
      cout << "No objects in bucket: " << bucket_name << endl;
    }
  }
  else {
    cout << "ERROR: " << kErrListObjectFailed
    << err_msg << endl;
    return 1;
  }
  return 0;
}

int cmdDeleteObject(Cdobs *const cdobs, string &bucket_name,
                    string &object_name, string &file_name) {
  if (file_name != "") {
    InvalidSyntax("-f not allowed with object delete");
    return 1;
  }
  else if (bucket_name == "") {
    InvalidSyntax("Missing required parameter -b BUCKET_NAME");
    return 1;
  }
  else if (object_name == "") {
    InvalidSyntax("Missing object name parameter");
    return 1;
  }

  string err_msg;
  int rc_del = cdobs->DeleteObject(bucket_name, object_name, err_msg);
  if (rc_del) {
    cout << "ERROR: " << kErrDeleteObjectFailed
    << err_msg << endl;
    return 1;
  }
  return 0;
}

int cmdObject (Cdobs *const cdobs, int argc, char **argv) {
  if (argc < 2) {
    InvalidSyntax("Missing OPERATION");
    return 0;
  }
  string bucket_name, file_name, object_name;
  for (int i = 2; i < argc; ++i) {
    if (!std::strcmp(argv[i], "-b")) {
      bucket_name = argv[i + 1];
      i++;
    }
    else if (!std::strcmp(argv[i], "-f")) {
      file_name = argv[i + 1];
      i++;
    }
    else if (i == argc - 1) {
      object_name = argv[i];
    }
    else {
      InvalidSyntax(argv[i]);
      return 1;
    }
  }
  string arg1(argv[1]);
  if (arg1 == "put") {
    return cmdPutObject(cdobs, bucket_name,
                        object_name, file_name);
  }
  else if (arg1 == "list") {
    return cmdListObjects(cdobs, bucket_name);
  }
  else if (arg1 == "delete") {
    return cmdDeleteObject(cdobs, bucket_name,
                          object_name, file_name);  
  }
  else {
    InvalidSyntax(arg1);
    return 1;
  }
}

// Named args are those that are
// specified with - or --, eg. --db
static
int GetNamedArg (char *p_arg) {
  if (p_arg[0] != '-') {
    return 0; // Not a named arg
  }
  char *arg = (p_arg[1] == '-') ? &p_arg[2]
      : &p_arg[1];
  for (int i = 1; i < named_arg_list.size(); ++i) {
    if (named_arg_list[i].first == arg) {
      dout << "Comp string with char * successful" << endl;
      return i;
    }
  }
  return -1;
}

static
int ParseArgs (int &argc, char **&argv, map<int, char *> &arg_map) {
  // Advance through 1 for name of prog
  // i.e cdobs
  argc--; ++argv;
  int i = 0, arg, incr = 0;
  while (i < argc) {
    arg = GetNamedArg(argv[i]);
    if (!arg) {
      argc -= i; argv += i;
      // Not a named arg
      return 0;
    }
    else if (arg == -1) {
      // Invalid named arg
      InvalidSyntax();
      return -1;
    }
    
    if (named_arg_list[arg].second) {
      incr = 2;
      if (i + 1 == argc) {
        InvalidSyntax();
        return -1;
      }
      if (!arg_map.insert(std::pair<int, char *>(
          arg, argv[i + 1])).second) {
        // Already exists entry, undefined.
        InvalidSyntax("Duplicate argument " + named_arg_list[arg].first);
        return -1;
      }        
    }
    else {
      incr = 1;
      char empty[] = "";
      arg_map.insert(std::pair<int, char *>(arg, empty));
    }
    
    i += incr;
  }

  argc -= i; argv += i;
  return 0;
}

// TODO: 
// 1. Add a help message.
// 2. Case insentivity, etc. 
// 2. Use a proper parser to parse
// arguments. 
int main(int argc, char **argv) {
  dout << "Starting program cdobs" << endl;
  int c_argc = argc; char **c_argv = argv;
  if (c_argc < 2) {
    Help();
    return 0;
  }

  map<int, char *> arg_map;
  int rc_parse = ParseArgs(c_argc, c_argv, arg_map);
  if (rc_parse) {
    return 1;
  }

  if (arg_map.count(HELP)) {
    Help();
    return 0;
  }

  string db_file;
  if (arg_map.count(DB)) {
    db_file = arg_map[DB];
  }
  else {
    db_file = kDefaultDbPath + kDefaultDbFile;
  }

  dout << "DB file is: " << db_file << endl;

  int exit_code = 0;  
  string cmd(c_argv[0]);
  if (cmd == "init") {
    exit_code = cmdInit(c_argc, c_argv, db_file);
  }
  else  {
    Cdobs *cdobs;
    int ret_value = 0;
    string err_msg;
    int rc_init = InitCdobs(&cdobs, db_file, err_msg);
    if (rc_init) {
      cout << "ERROR: " << err_msg << endl;
      exit_code = 1;
    }
    else {
      if (cmd == "bucket") {
        // send the rest of commands
        exit_code = cmdBucket(cdobs, c_argc, c_argv);
      }
      else if (cmd == "object") {
        exit_code = cmdObject(cdobs, c_argc, c_argv);
      }
      else {
        InvalidSyntax(cmd);
        exit_code = 1;
      }       
    }
  }

  return exit_code;
}
