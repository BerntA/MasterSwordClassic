#define VALVE_DLL 1

#include "MonsterList.h"

scriptres_t g_ScriptFiles[] =
{
	"monsters/bat.script", "bat",
	"monsters/boar.script", "boar",
	"monsters/giantbat.script", "giantbat",
	"monsters/giantrat.script", "giantrat",
	"monsters/giantspider.script", "giantspider",
	"monsters/gspider.script", "gspider",
	"monsters/hawk.script", "hawk",
	"monsters/minispider.script", "minispider",
	"monsters/orcarcher.script", "orcarcher",
	"monsters/orcberserker.script", "orcberserker",
	"monsters/orcranger.script", "orcranger",
	"monsters/orcwarrior.script", "orcwarrior",
	"monsters/skeleton.script", "skeleton",
	"monsters/skeleton2.script", "skeleton2",
	"monsters/spider.script", "spider",
	"dq/findlebind.script", "findlebind",
	"dq/murmur.script", "murmur",
	"dq/voldar.script", "voldar",
	"dq/voldarwarrior.script", "voldarwarrior",
};
int g_iScriptFilesSize = sizeof(g_ScriptFiles)/sizeof(g_ScriptFiles[0]);