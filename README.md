# Bus router

This is a C++ Bus Router application that is able to store bus stops and routes and identify shortest trips, in terms of time.

## Input/output examples

### AddStopRequest input example

```
{
	"type": "Stop",
	"road_distances": {
		"Marushkino": 3900
	},
	"longitude": 37.20829,
	"name": "Tolstopaltsevo",
	"latitude": 55.611087
}
```

### AddRouteRequest input example

```
{
	"type": "Bus",
	"name": "256",
	"stops": [
		"Biryulyovo Zapadnoye",
		"Biryusinka",
		"Universam",
		"Biryulyovo Tovarnaya",
		"Biryulyovo Passazhirskaya",
		"Biryulyovo Zapadnoye"
	],
	"is_roundtrip": true
}

```

### ReadRouteRequest input example

```
{
	"type": "Bus",
	"name": "750",
	"id": 519139350
}
```

### ReadStopRequest output example 

```
{
    "route_length": 5950,
    "request_id": 1965312327,
    "curvature": 1.36124,
    "stop_count": 6,
    "unique_stop_count": 5
  }
```

### ReadStopRequest input example

```
{
	"type": "Stop",
	"name": "Biryulyovo Zapadnoye",
	"id": 1042838872
}
```

### ReadStopRequest output example

```
{
    "buses": [
      "256",
      "828"
    ],
    "request_id": 1042838872
}
```

### ReadRouteSearchRequest input example

```
{
	"type": "Route",
	"from": "Biryulyovo Zapadnoye",
	"to": "Universam",
	"id": 4
}
```

### ReadRouteSearchRequest output example

```
{
	"total_time": 11.235,
	"items": [
		{
			"time": 6,
			"type": "Wait",
			"stop_name": "Biryulyovo Zapadnoye"
		},
		{
			"span_count": 2,
			"bus": "297",
			"type": "Bus",
			"time": 5.235
		}
	],
	"request_id": 4
}
```

### To run the project:

- Clone this project
- Add following task if you are using VS code:

```
{
			"type": "cppbuild",
			"label": "C/C++: clang++ build directory",
			"command": "/usr/bin/clang++",
			"args": [
				"--std=c++2a",
                "-stdlib=libc++",
				"-g",
				"${fileDirname}/*.cpp",
				"-o",
				"${fileDirname}/${fileBasenameNoExtension}"
			],
			"options": {
				"cwd": "${workspaceFolder}"
			},
			"problemMatcher": [
				"$gcc"
			],
			"group": "build",
			"isDefault": true,
			"detail": "compiler: /usr/bin/clang++"
		}
```

- Start task named *C/C++: clang++ build directory*
- Run ./main and observe results in output directory

