builtins.toJSON
  { a = 123;
    b = -456;
    c = "foo";
    d = "foo\n\"bar\"";
    e = true;
    f = false;
    g = [ 1 2 3 ];
    h = [ "a" [ "b" { "foo\nbar" = {}; } ] ];
    i = 1 + 2;
  }
