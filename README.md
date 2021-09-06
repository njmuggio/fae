# Fae - Text Template System

Fae is pretty minimal. Templates support the following:

* Variable subsitution
* Conditional blocks based on variable existence
* Loops to print the contents of STL-like containers

Any type that can be written to a `std::ostream` with the `<<` operator can can
be written to templates. Any STL-like container can be used in for loops.

Special formatting or extra logic must be performed by the host application.

When a template is created, it is compiled into a tiny bytecode program that
gets executed whenever the template is rendered.

All template commands occur between `$(` and `)`. Supported commands are listed
in the sections below.

## Supported Commands

### Variable Substitution

```html
<p>Hello, $(name)</p> <!-- Hello, Frank -->
<p>Hello, $(user)</p> <!-- Hello, Frank Castle (requires custom printing method for typeof(user)) -->
```

### Conditionals and Loops

```html
$(if names)
<ul>
  $(for n in names)
  <li>$(n)</li>
  $(end)
</ul>
$(end)
```

## History

Fae was started in August of 2021 to support [Aenthoril](https://aenthoril.com).
