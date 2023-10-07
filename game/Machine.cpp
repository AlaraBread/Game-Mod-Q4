#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

Machine

===============================================================================
*/

CLASS_DECLARATION(idEntity, Machine)
EVENT(EV_FindTargets, Machine::Event_FindTargets)
END_CLASS

/*
================
Machine::Machine
================
*/
Machine::Machine(void) {
}

/*
================
Machine::Spawn
================
*/
void Machine::Spawn(void) {
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	GetPhysics()->EnableClip();

	BecomeActive(TH_THINK | TH_PHYSICS);
}

/*
================
Machine::Think
================
*/
void Machine::Think(void) {
	trace_t pushResults;
	idVec3	newOrigin;
	idMat3	newAxis;
	idVec3	oldOrigin;
	idMat3	oldAxis;

	oldOrigin = GetPhysics()->GetOrigin();
	oldAxis = GetPhysics()->GetAxis();

	idVec3 dir;
	dir.Zero();
	dir.x = 1.0;

	newOrigin = GetPhysics()->GetOrigin() + dir * 1.0 * MS2SEC(gameLocal.GetMSec());
	newAxis = oldAxis;

	idEntity *item = gameLocal.push.ClipItems(this, 0, GetPhysics()->GetOrigin(), dir);

	if (item) {
		idDict spawnArgs = item->spawnArgs;
		bool consumed = true;
		idStr item_type = spawnArgs.GetString("factory_item", "");
		gameLocal.Printf("item type: %s\n", item_type.c_str());
		if (item_type == "red") {
			red++;
		} else {
			consumed = false;
		}
		if (consumed) {
			item->PostEventMS(&EV_Remove, 0);
		}
	}

	GetPhysics()->SetOrigin(oldOrigin);
	GetPhysics()->SetAxis(oldAxis);

	if (red >= 3) {
		red = 0;
		idDict args;
		args.Set("classname", "item_health_small_moveable");
		idVec3 org = oldOrigin;
		args.Set("origin", org.ToString());
		idEntity* item = NULL;
		gameLocal.SpawnEntityDef(args, &item, false);
	}

	idEntity::Think();
}

/*
================
Machine::Save
================
*/
void Machine::Save(idSaveGame* savefile) const
{
}

/*
================
Machine::Restore
================
*/
void Machine::Restore(idRestoreGame* savefile)
{
}

/*
================
Machine::Event_FindTargets
================
*/
void Machine::Event_FindTargets(void) {
	FindTargets();

	moveDir = GetPhysics()->GetAxis()[0];
	if (!targets.Num() || !targets[0]) {
		return;
	}

	idEntity* path;
	idEntity* next;
	path = targets[0];
	path->FindTargets();
	next = path->targets.Num() ? path->targets[0].GetEntity() : NULL;
	if (!next) {
		return;
	}

	moveDir = next->GetPhysics()->GetOrigin() - path->GetPhysics()->GetOrigin();
	moveDir.Normalize();

	path->PostEventMS(&EV_Remove, 0);
}
