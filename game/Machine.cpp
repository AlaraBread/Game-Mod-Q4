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

	red = 0;
	green = 0;
	blue = 0;

	crafter = spawnArgs.GetBool("crafter");
	extractor = spawnArgs.GetBool("extractor");

	timer = 0;

	BecomeActive(TH_THINK | TH_PHYSICS);
}

/*
================
Machine::Think
================
*/
void Machine::Think(void) {
	idVec3	oldOrigin;
	idMat3	oldAxis;

	oldOrigin = GetPhysics()->GetOrigin();
	oldAxis = GetPhysics()->GetAxis();

	idVec3 spawnPos = (oldOrigin + oldAxis * spawnArgs.GetVector("output_offset", "0 0 0"));

	if (extractor) {
		timer++;
		if (timer >= 100) {
			timer = 0;
			idDict args;
			args.Set("classname", spawnArgs.GetString("extract_classname"));
			args.Set("origin", spawnPos.ToString());
			idEntity* item = NULL;
			gameLocal.SpawnEntityDef(args, &item, false);
		}
	}

	// consume items
	idVec3	newOrigin;
	idMat3	newAxis;
	idVec3 dir;
	dir.Zero();
	dir.z = 1.0;

	newOrigin = GetPhysics()->GetOrigin() + dir;
	newAxis = oldAxis;
	idEntity* item = gameLocal.push.ClipItems(this, 0, GetPhysics()->GetOrigin(), dir);

	GetPhysics()->SetOrigin(oldOrigin);
	GetPhysics()->SetAxis(oldAxis);

	if (crafter) {
		if (item) {
			idDict spawnArgs = item->spawnArgs;
			bool consumed = true;
			idStr item_type = spawnArgs.GetString("factory_item", "");
			if (item_type == "red") {
				red++;
			} else if (item_type == "green") {
				green++;
			} else if (item_type == "blue") {
				blue++;
			} else {
				consumed = false;
			}
			if (consumed) {
				item->PostEventMS(&EV_Remove, 0);
			}
		}

		// craft items
		if (spawnArgs.GetBool("mixer")) {
			if (red >= 2 && green >= 2) {
				red = 0;
				green = 0;

				idDict args;
				args.Set("classname", "item_yellow");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			} else if (red >= 2 && blue >= 2) {
				red = 0;
				blue = 0;

				idDict args;
				args.Set("classname", "item_magenta");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			} else if (green >= 2 && blue >= 2) {
				green = 0;
				blue = 0;

				idDict args;
				args.Set("classname", "item_cyan");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			}
		}
		if (spawnArgs.GetBool("shifter")) {
			if (red >= 2) {
				red = 0;

				idDict args;
				args.Set("classname", "item_green");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			}
			else if (green >= 2) {
				green = 0;

				idDict args;
				args.Set("classname", "item_blue");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			}
			else if (blue >= 2) {
				blue = 0;

				idDict args;
				args.Set("classname", "item_red");
				args.Set("origin", spawnPos.ToString());
				idEntity* item = NULL;
				gameLocal.SpawnEntityDef(args, &item, false);
			}
		}
	}
	if (item) {
		idDict itemSpawnArgs = item->spawnArgs;
		if (itemSpawnArgs.GetBool("killme")) {
			idPlayer *player = gameLocal.GetLocalPlayer();
			if (player) {
				player->GiveInventoryItem(&spawnArgs);
			}
			item->PostEventMS(&EV_Remove, 0);
			PostEventMS(&EV_Remove, 0);
		}
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
