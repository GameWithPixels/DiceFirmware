# Pixels Firmware Coding Style Guidelines

## Names:
- Use PascalCase for all type names (enums, structs, classes) (need to change some)
- Use PascalCase for namespaces
- Use camelCase for functions (need to change some)
- Use camelCase for property names, local variables
- Use snake_case for file names
- Use UPPER_CASE for macros, defines

## Components:
- One file per component
- One header file per source file

## Switch Statements:
- `switch` statements should use braces for blocks
- Should always have a default case

## Return Statements:
- Only use parentheses in return statements when you would in x = expr;
    ```c++
    return result;                  //No need for parentheses
    return (LONG_CONDITION_1 &&
            LONG_CONDITION_2);      // Parentheses O.K.
    ```

## Error Checking/Handling:
- Return a code of ret_type_t within functions that can fail
- Use `APP_ERROR_CHECK(ret)` to check this error code 
- `app_error_handler` will automatically be called if ret != NRF_SUCCESS
- Check error type within handling function and handle appropriately

## Comments:
- Use doxygen style comments
- Don't use /* for comments inside functions
- Use summary tags for high-level descriptions
    ```c++
    /// <summary>
	/// "High-level description of function/module"
	/// </summary>
    ```
- Use inline comments for lower-level descriptions
    ```c++
    foo(); //Lower-level description of foo's purpose
    ```

## Spaces vs. Tabs:
- Do not use tabs!
- Use 4 spaces for each indentation

## Style:
- Open curly braces always go on line below whatever necessitates them
- Always surround loop and conditional bodies with curly braces unless on same line
- Single space should follow commas, colons, and semicolons in those constructs
    - `for (int i = 0; i < 10; i++) `
- `else` statements go on next line from closing curly brace
- No assignments within expressions, I.E. `if (a = b)...`
- Keep lines under 100 characters

## Commit Messages:
- Always use present tense in commit messages

