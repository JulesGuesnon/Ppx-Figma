let dirExists = path =>
  try(Sys.is_directory(path)) {
  | Sys_error(_) => false
  };

let getTempPath = () => {
  switch (Sys.os_type) {
  | "Unix" => Sys.getenv("TMPDIR")
  | "Win32" => Sys.getenv("TMP")
  | _ => raise(Not_found)
  };
};

let createDirIfNotExists = path =>
  if (dirExists(path)) {
    ();
  } else {
    Unix.mkdir(path, 0o777);
  };