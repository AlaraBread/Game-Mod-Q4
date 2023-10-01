#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


/*
===============================================================================

  Machine

===============================================================================
*/

const idEventDef EV_DropToFloor("<dropToFloor>");

CLASS_DECLARATION(idEntity, Machine)

EVENT(EV_Activate, Machine::Event_Trigger)
EVENT(EV_SetGravity, Machine::Event_SetGravity)
EVENT(EV_DropToFloor, Machine::Event_DropToFloor)

END_CLASS



/*
================
Machine::Machine
================
*/
Machine::Machine() {
	spin = false;
	inView = false;
	skin = NULL;
	pickupSkin = NULL;
	inViewTime = 0;
	lastCycle = 0;
	lastRenderViewTime = -1;
	itemShellHandle = -1;
	shellMaterial = NULL;
	orgOrigin.Zero();
	canPickUp = true;
	fl.networkSync = true;
	trigger = NULL;
	syncPhysics = false;
	srvReady = -1;
	clReady = -1;
	effectIdle = NULL;
	itemPVSArea = 0;
	effectIdle = NULL;
	simpleItem = false;
	pickedUp = false;
}

/*
================
Machine::~Machine
================
*/
Machine::~Machine() {
	// remove the highlight shell
	if (itemShellHandle != -1) {
		gameRenderWorld->FreeEntityDef(itemShellHandle);
	}
	if (trigger) {
		delete trigger;
	}

	SetPhysics(NULL);
}

/*
================
Machine::Save
================
*/
void Machine::Save(idSaveGame* savefile) const {
	savefile->WriteClipModel(trigger);
	savefile->WriteBool(spin);

	savefile->WriteSkin(skin);
	savefile->WriteSkin(pickupSkin);

	savefile->WriteVec3(orgOrigin);

	savefile->WriteBool(pulse);
	savefile->WriteBool(canPickUp);

	savefile->WriteStaticObject(physicsObj);

	//	savefile->WriteInt(itemShellHandle);	// cnicholson: Set at end of Restore, do not save
	savefile->WriteMaterial(shellMaterial);

	savefile->WriteBool(inView);
	savefile->WriteInt(inViewTime);
	savefile->WriteInt(lastCycle);
	savefile->WriteInt(lastRenderViewTime);
}

/*
================
Machine::Restore
================
*/
void Machine::Restore(idRestoreGame* savefile) {
	savefile->ReadClipModel(trigger);
	savefile->ReadBool(spin);

	savefile->ReadSkin(skin);
	savefile->ReadSkin(pickupSkin);

	savefile->ReadVec3(orgOrigin);

	savefile->ReadBool(pulse);
	savefile->ReadBool(canPickUp);

	savefile->ReadStaticObject(physicsObj);

	//	savefile->ReadInt(itemShellHandle);	// cnicholson: Set at end of function, do not restore
	savefile->ReadMaterial(shellMaterial);

	savefile->ReadBool(inView);
	savefile->ReadInt(inViewTime);
	savefile->ReadInt(lastCycle);
	savefile->ReadInt(lastRenderViewTime);

	RestorePhysics(&physicsObj);

	physicsObj.SetSelf(this);

	itemShellHandle = -1;
}

/*
================
Machine::UpdateRenderEntity
================
*/
bool Machine::UpdateRenderEntity(renderEntity_s* renderEntity, const renderView_t* renderView) const {
	if (simpleItem) {
		return false;
	}

	if (lastRenderViewTime == renderView->time) {
		return false;
	}

	lastRenderViewTime = renderView->time;

	// check for glow highlighting if near the center of the view
	idVec3 dir = renderEntity->origin - renderView->vieworg;
	dir.Normalize();
	float d = dir * renderView->viewaxis[0];

	// two second pulse cycle
	float cycle = (renderView->time - inViewTime) / 2000.0f;

	if (d > 0.94f) {
		if (!inView) {
			inView = true;
			if (cycle > lastCycle) {
				// restart at the beginning
				inViewTime = renderView->time;
				cycle = 0.0f;
			}
		}
	}
	else {
		if (inView) {
			inView = false;
			lastCycle = ceil(cycle);
		}
	}

	// fade down after the last pulse finishes 
	if (!inView && cycle > lastCycle) {
		renderEntity->shaderParms[4] = 0.0f;
	}
	else {
		// pulse up in 1/4 second
		cycle -= (int)cycle;
		if (cycle < 0.1f) {
			renderEntity->shaderParms[4] = cycle * 10.0f;
		}
		else if (cycle < 0.2f) {
			renderEntity->shaderParms[4] = 1.0f;
		}
		else if (cycle < 0.3f) {
			renderEntity->shaderParms[4] = 1.0f - (cycle - 0.2f) * 10.0f;
		}
		else {
			// stay off between pulses
			renderEntity->shaderParms[4] = 0.0f;
		}
	}

	// update every single time this is in view
	return true;
}

/*
================
Machine::UpdateTrigger
================
*/
void Machine::UpdateTrigger(void) {
	// update trigger position
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	trigger->Link(this, 0, GetPhysics()->GetOrigin(), mat3_identity);
	// RAVEN END
}

/*
================
Machine::ModelCallback
================
*/
bool Machine::ModelCallback(renderEntity_t* renderEntity, const renderView_t* renderView) {
	const Machine* ent;

	// this may be triggered by a model trace or other non-view related source
	if (!renderView) {
		return false;
	}

	ent = static_cast<Machine*>(gameLocal.entities[renderEntity->entityNum]);
	if (!ent) {
		gameLocal.Error("Machine::ModelCallback: callback with NULL game entity");
	}

	return ent->UpdateRenderEntity(renderEntity, renderView);
}


/*
================
Machine::GetPhysicsToVisualTransform
================
*/
bool Machine::GetPhysicsToVisualTransform(idVec3& origin, idMat3& axis) {
	if (simpleItem) {
		if (gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetRenderView()) {
			if (gameLocal.GetLocalPlayer()->spectating) {
				idPlayer* spec = (idPlayer*)gameLocal.entities[gameLocal.GetLocalPlayer()->spectator];
				if (spec && spec->GetRenderView()) {
					axis = spec->GetRenderView()->viewaxis;
				}
			}
			else {
				axis = gameLocal.GetLocalPlayer()->GetRenderView()->viewaxis;
			}
		}
		else {
			// dedicated server for instance
			axis = mat3_identity;
		}
		origin = idVec3(0.0f, 0.0f, 32.0f);
		return true;
	}

	if (!spin || (gameLocal.isServer && !gameLocal.isListenServer)) {
		return false;
	}

	idAngles ang;
	ang.pitch = ang.roll = 0.0f;
	ang.yaw = (gameLocal.time & 4095) * 360.0f / -4096.0f;
	axis = ang.ToMat3() * GetPhysics()->GetAxis();

	float scale = 0.005f;
	float offset = entityNumber * 0.685145f;		// rjohnson: just a random number here to shift the cos curve

	origin.Zero();

	origin += GetPhysics()->GetAxis()[2] * (4.0f + idMath::Cos(((gameLocal.time + 1000) * scale) + offset) * 4.0f);

	return true;
}

void Machine::Event_DropToFloor(void) {
	// don't drop the floor if bound to another entity
	if (GetBindMaster() != NULL && GetBindMaster() != this) {
		return;
	}

	physicsObj.DropToFloor();
}

// RAVEN BEGIN
// mekberg: added
/*
================
Machine::Collide
================
*/
bool Machine::Collide(const trace_t& collision, const idVec3& velocity) {
	idEntity* lol = gameLocal.entities[collision.c.entityNum];
	if (gameLocal.isMultiplayer && collision.c.contents & CONTENTS_ITEMCLIP && lol && !lol->IsType(Machine::GetClassType())) {
		PostEventMS(&EV_Remove, 0);
	}
	return false;
}
// RAVEN END

/*
================
Machine::Think
================
*/
void Machine::Think(void) {
	if (thinkFlags & TH_PHYSICS) {
		RunPhysics();
		UpdateTrigger();
	}

	if (gameLocal.IsMultiplayer() && g_skipItemShadowsMP.GetBool()) {
		renderEntity.suppressShadowInViewID = gameLocal.localClientNum + 1;
	}
	else {
		renderEntity.suppressShadowInViewID = 0;
	}
	for (int i = 0; i < physicsObj.GetNumContacts(); i++) {
		contactInfo_t contact = physicsObj.GetContact(i);
		idEntity *entity = gameLocal.entities[contact.entityNum];
		if (entity) {
			idVec3 p;
			idVec3 f;
			f.x = -10.0;
			entity->AddForce(this, 0, p, f);
			gameLocal.Printf("adding force to entity\n");
		}
	}

	if (!(simpleItem && pickedUp)) {
		UpdateVisuals();
		Present();
	}
}

/*
================
Machine::Present
================
*/
void Machine::Present(void) {
	idEntity::Present();

	if (!fl.hidden && pulse) {
		// also add a highlight shell model
		renderEntity_t	shell;

		shell = renderEntity;

		// we will mess with shader parms when the item is in view
		// to give the "item pulse" effect
		shell.callback = Machine::ModelCallback;
		shell.entityNum = entityNumber;
		shell.customShader = shellMaterial;
		if (itemShellHandle == -1) {
			itemShellHandle = gameRenderWorld->AddEntityDef(&shell);
		}
		else {
			gameRenderWorld->UpdateEntityDef(itemShellHandle, &shell);
		}
	}
}

/*
================
Machine::InstanceJoin
================
*/
void Machine::InstanceJoin(void) {
	idEntity::InstanceJoin();

	UpdateModelTransform();
	if (!simpleItem && spawnArgs.GetString("fx_idle")) {
		PlayEffect("fx_idle", renderEntity.origin, renderEntity.axis, true);
	}
}

/*
================
Machine::InstanceLeave
================
*/
void Machine::InstanceLeave(void) {
	idEntity::InstanceLeave();

	StopEffect("fx_idle", true);
}

/*
================
Machine::Spawn
================
*/
void Machine::Spawn(void) {
	idStr		giveTo;
	idEntity* ent;
	idVec3		vSize;
	idVec3 size;
	size.x = 3.0;
	size.y = 1.0;
	size.z = 3.0;
	idBounds	bounds(vec3_origin - size, vec3_origin + size);

	// check for triggerbounds, which allows for non-square triggers (useful for, say, a CTF flag)	 
	if (spawnArgs.GetVector("triggerbounds", "16 16 16", vSize)) {
		bounds.AddPoint(idVec3(vSize.x * 0.5f, vSize.y * 0.5f, 0.0f));
		bounds.AddPoint(idVec3(-vSize.x * 0.5f, -vSize.y * 0.5f, vSize.z));
	}
	else {
		// create a square trigger for item pickup
		float tsize;
		spawnArgs.GetFloat("triggersize", "16.0", tsize);
		bounds.ExpandSelf(tsize);
	}

	// RAVEN BEGIN
	// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
	// RAVEN END
	trigger = new idClipModel(idTraceModel(bounds));
	// RAVEN BEGIN
	// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
	// RAVEN END
	// RAVEN BEGIN
	// ddynerman: multiple clip worlds
	trigger->Link(this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis());
	// RAVEN END

	physicsObj.SetSelf(this);
	// RAVEN BEGIN
	// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
	// RAVEN END

	physicsObj.SetClipModel(new idClipModel(GetPhysics()->GetClipModel()), 1.0f);

	// RAVEN BEGIN
	// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
	// RAVEN END
	physicsObj.SetOrigin(GetPhysics()->GetOrigin());
	physicsObj.SetAxis(GetPhysics()->GetAxis());
	physicsObj.SetGravity(gameLocal.GetGravity());
	physicsObj.SetContents(0);
	physicsObj.SetClipMask(MASK_SOLID);
	physicsObj.SetFriction(0.0f, 0.0f, 6.0f);
	physicsObj.SetClipBox(bounds, 1.0);
	SetPhysics(&physicsObj);

	if (spawnArgs.GetBool("start_off")) {
		trigger->SetContents(0);
		Hide();
	}
	else {
		trigger->SetContents(CONTENTS_TRIGGER);
	}

	giveTo = spawnArgs.GetString("owner");
	if (giveTo.Length()) {
		ent = gameLocal.FindEntity(giveTo);
		if (!ent) {
			gameLocal.Error("Item couldn't find owner '%s'", giveTo.c_str());
		}
		PostEventMS(&EV_Touch, 0, ent, NULL);
	}

	if (spawnArgs.GetBool("spin") || gameLocal.isMultiplayer) {
		spin = true;
		BecomeActive(TH_THINK);
	}

	// pulse ( and therefore itemShellHandle ) was taken out and shot. do not sync
	//pulse = !spawnArgs.GetBool( "nopulse" );
	pulse = false;
	orgOrigin = GetPhysics()->GetOrigin();

	canPickUp = !(spawnArgs.GetBool("triggerFirst") || spawnArgs.GetBool("no_touch"));

	inViewTime = -1000;
	lastCycle = -1;
	itemShellHandle = -1;
	// RAVEN BEGIN
	// abahr: move texture to def file for precaching
	shellMaterial = declManager->FindMaterial(spawnArgs.GetString("mtr_highlight", "_default"));
	PostEventMS(&EV_SetGravity, 0);
	// RAVEN END
	if (spawnArgs.GetString("skin", NULL)) {
		skin = declManager->FindSkin(spawnArgs.GetString("skin"), false);
		if (skin) {
			SetSkin(skin);
			srvReady = 1;
		}
	}
	else {
		skin = NULL;
	}

	if (spawnArgs.GetString("skin_pickup", NULL)) {
		pickupSkin = declManager->FindSkin(spawnArgs.GetString("skin_pickup"), false);
	}
	else {
		pickupSkin = NULL;
	}

	syncPhysics = spawnArgs.GetBool("net_syncPhysics", "0");

	if (srvReady == -1) {
		srvReady = IsHidden() ? 0 : 1;
	}

	// RAVEN BEGIN
	// mekberg: added for removing pickups in mp in pits
	if (gameLocal.isMultiplayer) {
		trigger->SetContents(trigger->GetContents() | CONTENTS_ITEMCLIP);
	}
	// RAVEN END

	if (gameLocal.isMultiplayer) {
		itemPVSArea = gameLocal.pvs.GetPVSArea(GetPhysics()->GetOrigin());
	}
	else {
		itemPVSArea = 0;
	}

	simpleItem = g_simpleItems.GetBool() && gameLocal.isMultiplayer && !IsType(rvItemCTFFlag::GetClassType());
	if (simpleItem) {
		memset(&renderEntity, 0, sizeof(renderEntity));
		renderEntity.axis = mat3_identity;
		renderEntity.shaderParms[SHADERPARM_RED] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_GREEN] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_BLUE] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_SPRITE_WIDTH] = 32.0f;
		renderEntity.shaderParms[SHADERPARM_SPRITE_HEIGHT] = 32.0f;
		renderEntity.hModel = renderModelManager->FindModel("_sprite");
		renderEntity.callback = NULL;
		renderEntity.numJoints = 0;
		renderEntity.joints = NULL;
		renderEntity.customSkin = 0;
		renderEntity.noShadow = true;
		renderEntity.noSelfShadow = true;
		renderEntity.customShader = declManager->FindMaterial(spawnArgs.GetString("mtr_simple_icon"));

		renderEntity.referenceShader = 0;
		renderEntity.bounds = renderEntity.hModel->Bounds(&renderEntity);
		SetAxis(mat3_identity);
	}
	else {
		if (spawnArgs.GetString("fx_idle")) {
			UpdateModelTransform();
			effectIdle = PlayEffect("fx_idle", renderEntity.origin, renderEntity.axis, true);
		}
	}

	GetPhysics()->SetClipMask(GetPhysics()->GetClipMask() | CONTENTS_ITEMCLIP);
	pickedUp = false;
}

/*
================
Machine::Event_SetGravity
================
*/
void Machine::Event_SetGravity() {
	// If the item isnt a dropped item then see if it should settle itself
	// to the floor or not
	if (!spawnArgs.GetBool("dropped")) {
		if (spawnArgs.GetBool("nodrop")) {
			physicsObj.PutToRest();
		}
		else {
			PostEventMS(&EV_DropToFloor, 0);
		}
	}
}
// RAVEN END

/*
================
Machine::GetAttributes
================
*/
void Machine::GetAttributes(idDict& attributes) {
	int					i;
	const idKeyValue* arg;

	for (i = 0; i < spawnArgs.GetNumKeyVals(); i++) {
		arg = spawnArgs.GetKeyVal(i);
		if (arg->GetKey().Left(4) == "inv_") {
			attributes.Set(arg->GetKey().Right(arg->GetKey().Length() - 4), arg->GetValue());
		}
	}
}

/*
================
Machine::Hide
================
*/
void Machine::Hide(void) {
	srvReady = 0;
	idEntity::Hide();
	trigger->SetContents(0);
}

/*
================
Machine::Show
================
*/
void Machine::Show(void) {
	srvReady = 1;
	idEntity::Show();
	trigger->SetContents(CONTENTS_TRIGGER);
}

/*
================
Machine::ClientStale
================
*/
bool Machine::ClientStale(void) {
	idEntity::ClientStale();

	StopEffect("fx_idle");
	return false;
}

/*
================
Machine::ClientUnstale
================
*/
void Machine::ClientUnstale(void) {
	idEntity::ClientUnstale();

	UpdateModelTransform();
	if (!simpleItem && spawnArgs.GetString("fx_idle")) {
		PlayEffect("fx_idle", renderEntity.origin, renderEntity.axis, true);
	}
}

/*
================
Machine::ClientPredictionThink
================
*/
void Machine::ClientPredictionThink(void) {
	// only think forward because the state is not synced through snapshots
	if (!gameLocal.isNewFrame) {
		return;
	}
	Think();
}

/*
================
Machine::WriteFromSnapshot
================
*/
void Machine::WriteToSnapshot(idBitMsgDelta& msg) const {
	if (syncPhysics) {
		physicsObj.WriteToSnapshot(msg);
	}
	assert(srvReady != -1);
	msg.WriteBits((srvReady == 1), 1);
}

/*
================
Machine::ReadFromSnapshot
================
*/
void Machine::ReadFromSnapshot(const idBitMsgDelta& msg) {
	if (syncPhysics) {
		physicsObj.ReadFromSnapshot(msg);
	}
	int newReady = (msg.ReadBits(1) != 0);
	idVec3 resetOrigin(0, 0, 0);
	// client spawns the ent with ready == -1 so the state set happens at least once
	if (newReady != clReady) {
		if (newReady) {
			// g_simpleItems might force a hide even with a pickup skin
			if (pickupSkin) {
				SetSkin(skin);
			}
			else {
				SetSkin(skin);
				Show();
			}

			if (simpleItem) {
				UpdateVisuals();
			}
			pickedUp = false;

			if (effectIdle.GetEntity()) {
				UpdateModelTransform();
				effectIdle->SetOrigin(resetOrigin);
			}
			else if (spawnArgs.GetString("fx_idle") && !simpleItem) {
				UpdateModelTransform();
				effectIdle = PlayEffect("fx_idle", renderEntity.origin, renderEntity.axis, true);
			}
		}
		else {
			if (pickupSkin) {
				SetSkin(pickupSkin);
			}
			else {
				Hide();
			}

			if (simpleItem) {
				FreeModelDef();
				UpdateVisuals();
			}

			pickedUp = true;

			StopEffect("fx_idle");
			effectIdle = NULL;
		}
	}
	clReady = newReady;
}

/*
================
Machine::ClientReceiveEvent
================
*/
bool Machine::ClientReceiveEvent(int event, int time, const idBitMsg& msg) {
	switch (event) {
	default: {
		return idEntity::ClientReceiveEvent(event, time, msg);
	}
	}
	//unreachable
	//	return false;
}

/*
================
Machine::Event_Trigger
================
*/
void Machine::Event_Trigger(idEntity* activator) {
	if (!canPickUp && spawnArgs.GetBool("triggerFirst")) {
		canPickUp = true;
		return;
	}

	// RAVEN BEGIN
	// jnewquist: Use accessor for static class type 
	if (activator && activator->IsType(idPlayer::GetClassType())) {
		// RAVEN END
		//Pickup(static_cast<idPlayer*>(activator));
	}
}
