# Summary

1. [Git](#git)
   - [Commit](#commit)
   - [Flow](#flow)
2. [Code](#code) 
   - [Notations](#notations)
   - [Includes order](#includes-order)


# Git

---

## Commit

A commit is of the form:
- an action in braces
- a brief description

Example:  
{Update} Maths for cube collisions

## Flow

```mermaid
gitGraph
    commit id: "Init"
    branch develop
    commit
    branch feature1
    commit
    checkout develop
    branch feature2
    commit
    checkout develop
    merge feature1
    commit
    checkout main
    merge develop
    checkout develop
    commit
    branch featureN
    commit
    checkout feature2
    commit
    checkout develop
    merge feature2
    commit
    checkout main
    merge develop
    checkout develop
    merge featureN
    commit
    checkout main
    merge develop
```


# Code

---

## Notations

Notation        | Element
-------         | ------
m_camelCase     | class members
camelCase       | other variables
PascalCase      | functions / structures / classes
snake_case      | files
CAPITAL         | enum

## Includes order

```c++
#include "my_file.h" // just in my_file.cpp

#include <standard libraries>

#include "other_files.h"
```

<br>

[Head of page](#summary)