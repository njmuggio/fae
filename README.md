# Fae - Text Template System

* Fantastically Absurd Engine
* Frankly Awful Enigma

## Template Features

### Variable Substitution

```html
<p>Hello, [[$name]]</p> <!-- Hello, Frank -->
<p>Hello, [[$user.$firstName]] [[$user.$lastName]]</p> <!-- Hello, Frank Castle -->
```

### Null/Empty Coalescing

Null coalescing uses the `??` operator:

```html
<p>Hello, [[$name ?? 'unknown']]</p>
```

Empty coalescing uses the `?:` operator:

```html
<p>Hello, [[$name ?: 'unknown']]</p>
```

## Template Bytecode

Opcodes are two bytes, stored in the native endianness. The opcode format is as
follows:

```
F E D C B A 9 8 7 6 5 4 3 2 1 0
| OP  | |       VALUE         |
```

The following table lists the current opcode set:

| Opcode | Name     | Brief Description                                        |
| :----: | :------- | :------------------------------------------------------- |
| 0x0    | Halt     | VM execution is complete                                 |
| 0x1    | Push     | Push `VALUE` to the stack                                |
| 0x2    | Append   | Append a fragment to the string                          |
| 0x3    | VarSub   | Append a variable to the string                          |
| 0x4    | TExec    |
| 0x5    |
| 0x6    |
| 0x7    |
| 0x8    |
| 0x9    |
| 0xA    |
| 0xB    |
| 0xC    |
| 0xD    |
| 0xE    |
| 0xF    |
