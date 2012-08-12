function WorkerInit() {
	Module['print'] = function(str) {
		postMessage({type: "error", value: str});
	}

	var cwrap = Module['cwrap'];

	var ShInitialize = cwrap("ShInitialize");
	var ShInitBuiltInResources = cwrap("ShInitBuiltInResources", "void", ["number"]);
	var ShConstructCompiler = cwrap("ShConstructCompiler", "number", ["number", "number", "number", "number"]);
	var ShCompile = cwrap("ShCompile", "number", ["number", "number", "number", "number"]);
	var ShFinalize = cwrap("ShFinalize", "number");
	var ShGetInfo = cwrap("ShGetInfo", "void", ["number", "number", "number"]);
	var ShGetObjectCode = cwrap("ShGetObjectCode", "void", ["number", "number"]);
	var ShGetInfoLog = cwrap("ShGetInfoLog", "void", ["number", "number"]);

	var SH_VERTEX_SHADER = 0x8B31;
	var SH_WEBGL_SPEC = 0x8B41;
	var SH_CSS_SHADERS_SPEC = 0x8B42
	var SH_ESSL_OUTPUT = 0x8B45;
	var SH_OBJECT_CODE = 0x0004;
	var SH_INFO_LOG_LENGTH = 0x8B84;
	var SH_OBJECT_CODE_LENGTH = 0x8B88;
	var SH_INTERMEDIATE_TREE = 0x0002;
	var SH_JS_OUTPUT = 0x8B48;

	ShInitialize();
	var builtinResources = allocate(100 * 4, 'i8'); // big enough to cover for the size of ShBuiltInResources
	ShInitBuiltInResources(builtinResources);

	var compiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_CSS_SHADERS_SPEC, SH_JS_OUTPUT, builtinResources);

	function compile(shaderString) {
		var shaderStringPtr = allocate(intArrayFromString(shaderString), 'i8', ALLOC_STACK);
		var strings = allocate(4, 'i32', ALLOC_STACK);
		setValue(strings, shaderStringPtr, 'i32');
		var compileResult = ShCompile(compiler, strings, 1, SH_OBJECT_CODE);
		
		var result = {
			original: shaderString,
			compileResult: compileResult
		};

		ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, strings);
		var length = getValue(strings, 'i32');
		var shaderResultStringPtr = allocate(length, 'i8', ALLOC_STACK);
		ShGetObjectCode(compiler, shaderResultStringPtr);
		result.source = length > 1 ? Pointer_stringify(shaderResultStringPtr, length - 1) : "";
		
		ShGetInfo(compiler, SH_INFO_LOG_LENGTH, strings);
		var length = getValue(strings, 'i32');
		var shaderResultStringPtr = allocate(length, 'i8', ALLOC_STACK);
		ShGetInfoLog(compiler, shaderResultStringPtr);
		result.info = length > 1 ? Pointer_stringify(shaderResultStringPtr, length - 1) : "";

		return result;
	}

	this.onmessage = function(event) {
		switch (event.data.type) {
			case "compile":
				//try {
					var result = compile(event.data.source);
					postMessage({
						type: "result",
						result: result
					});
				// } catch (e) {
				// 	postMessage({type: "error", error: e.stack});
				// }
			break;
		}
	}
	postMessage({type: 'loaded'});
}
WorkerInit();
