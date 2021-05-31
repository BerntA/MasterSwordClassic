struct scriptres_s {
	const char *ScriptName;
	const char *ResName;
};

typedef struct scriptres_s scriptres_t;
extern scriptres_t g_ScriptFiles[];
extern int g_iScriptFilesSize;
