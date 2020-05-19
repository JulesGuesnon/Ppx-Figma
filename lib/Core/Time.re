type unit =
  | MS
  | S
  | M
  | H
  | D
  | Mon
  | Y;

exception BadUnit(string);

type t = {
  duration: float,
  unit,
};

exception ParseTime(string);

let getUnitMultiplier = unit => {
  switch (unit) {
  | MS => 0.001
  | S => 1.
  | M => 60.
  | H => 3600.
  | D => 3600. *. 24.
  | Mon => 3600. *. 24. *. 31.
  | Y => 3600. *. 24. *. 365.
  };
};

let unitOfString = str =>
  switch (str |> String.lowercase_ascii) {
  | "ms" => MS
  | "s" => S
  | "m" => M
  | "h" => H
  | "d" => D
  | "mon" => Mon
  | "y" => Y
  | s =>
    raise(
      BadUnit(
        Printf.sprintf("The following unit: '%s' is not a valid one", s),
      ),
    )
  };

let unitToString = unit =>
  switch (unit) {
  | MS => "ms"
  | S => "s"
  | M => "m"
  | H => "h"
  | D => "d"
  | Mon => "mon"
  | Y => "y"
  };

let regex = Str.regexp("^\\([0-9]+\\)\\([a-zA-Z]+\\)$");

let ofString = str => {
  if (!Str.string_match(regex, str, 0)) {
    raise(ParseTime("Couldn't parse the following time: " ++ str));
  };

  {
    duration: Str.matched_group(1, str) |> float_of_string,
    unit: Str.matched_group(2, str) |> unitOfString,
  };
};

let toString = t =>
  Printf.sprintf("%i%s", int_of_float(t.duration), unitToString(t.unit));

let getTimeInSeconds = t => t.duration *. getUnitMultiplier(t.unit);