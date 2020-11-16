module List = {
  let mapOrDefault = (cb, default, value) => {
    switch (value) {
    | Some(v) => v |> List.map(cb)
    | None => default
    };
  };
};

module Float = {
  let valueOrDefault = (default, value) =>
    switch (value) {
    | Some(v) => v
    | None => default
    };
};