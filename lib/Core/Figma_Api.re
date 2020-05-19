open Cohttp;
open Cohttp_lwt_unix;
open Lwt.Infix;

module Cache = {
  let dirPath = Printf.sprintf("%s/.cache", Sys.getcwd());
  let fileRegex = Str.regexp("\\([0-9]+\\)-\\([0-9]+\\)\\([a-zA-Z]+\\).json");

  let createCacheDirIfNotExists = () =>
    if (!Utils.Fs.dirExists(dirPath)) {
      Unix.mkdir(dirPath, 0o777);
    };

  let getFullpathOfFile = filename =>
    Printf.sprintf("%s/%s", dirPath, filename);

  let getCacheFilename = () => {
    let files = Sys.readdir(dirPath);

    if (files |> Array.length == 0) {
      None;
    } else {
      switch (
        files
        |> Array.to_list
        |> List.find(filename => Str.string_match(fileRegex, filename, 0))
      ) {
      | v => Some(v)
      | exception _ => None
      };
    };
  };

  let isCacheFileExpired = time => {
    switch (getCacheFilename()) {
    | Some(cacheFilename) =>
      switch (
        {
          Str.string_match(fileRegex, cacheFilename, 0) |> ignore;
          let expirationStr = Str.matched_group(0, cacheFilename);
          let fileTime = Str.matched_group(1, cacheFilename);

          let expiration =
            try(float_of_string(expirationStr)) {
            | _ => 0.
            };

          expiration < Unix.time() || time != fileTime;
        }
      ) {
      | v => v
      | exception _ => true
      }

    | None => true
    };
  };

  let cacheBody = (body, time) => {
    let expirationTime = Unix.time() +. Time.getTimeInSeconds(time);

    Utils.Fs.createDirIfNotExists(dirPath);

    let filePath =
      Printf.sprintf(
        "%i-%s",
        int_of_float(expirationTime),
        Time.toString(time),
      )
      ++ ".json"
      |> getFullpathOfFile;

    let chan = open_out(filePath);

    output_string(chan, body);

    close_out(chan);
  };

  let get = time =>
    if (!Utils.Fs.dirExists(dirPath)) {
      None;
    } else {
      switch (getCacheFilename(), isCacheFileExpired(time)) {
      | (Some(filename), false) =>
        let fullPath = getFullpathOfFile(filename);

        let chan = open_in(fullPath);
        let content = input_line(chan);
        close_in(chan);

        Some(content);

      | (_, true) =>
        switch (getCacheFilename()) {
        | Some(f) => getFullpathOfFile(f) |> Sys.remove
        | None => ()
        };
        None;
      | _ => None
      };
    };
};

let getFile = (~fileId, ~token, ~time, ()) => {
  let body =
    switch (time |> Time.toString |> Cache.get) {
    | Some(b) => b
    | None =>
      let headers = Header.of_list([("X-Figma-Token", token)]);
      let req =
        Client.get(
          ~headers,
          Uri.of_string(
            Printf.sprintf("https://api.figma.com/v1/files/%s", fileId),
          ),
        )
        >>= (
          ((_resp, body)) => {
            body |> Cohttp_lwt.Body.to_string >|= (body => body);
          }
        );

      let body = Lwt_main.run(req);

      Cache.cacheBody(body, time);

      body;
    };

  body;
};