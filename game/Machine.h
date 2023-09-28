#ifndef __GAME_MACHINE_H__
#define __GAME_MACHINE_H__

class Machine : public idItem {
public:
	CLASS_PROTOTYPE(Machine);

	Machine();
	virtual					~Machine();

	void					Save(idSaveGame* savefile) const;
	void					Restore(idRestoreGame* savefile);

	void					Spawn(void);
	void					GetAttributes(idDict& attributes);
	virtual void			Think(void);
	virtual void			Present();
	virtual	void			InstanceJoin(void);
	virtual void			InstanceLeave(void);
	virtual bool			GetPhysicsToVisualTransform(idVec3& origin, idMat3& axis);

	// RAVEN BEGIN
	// mekberg: added
	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);
	// RAVEN END

	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_RESPAWNFX,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink(void);
	virtual bool			ClientReceiveEvent(int event, int time, const idBitMsg& msg);

	// networking
	virtual void			WriteToSnapshot(idBitMsgDelta& msg) const;
	virtual void			ReadFromSnapshot(const idBitMsgDelta& msg);

	virtual void			Hide(void);
	virtual void			Show(void);

	virtual bool			ClientStale(void);
	virtual void			ClientUnstale(void);

	int						IsVisible() { return srvReady; }

	rvClientEntityPtr<rvClientEffect>	effectIdle;
	bool					simpleItem;
	bool					pickedUp;
	const idDeclSkin* pickupSkin;
protected:

	void					UpdateTrigger(void);
	void					SendPickupMsg(int clientNum);

	idClipModel* trigger;
	bool					spin;
	// only a small subset of Machine need their physics object synced
	bool					syncPhysics;

	bool					pulse;
	bool					canPickUp;
	const idDeclSkin* skin;

private:
	idVec3					orgOrigin;

	rvPhysics_Particle		physicsObj;

	// for item pulse effect
	int						itemShellHandle;
	const idMaterial* shellMaterial;

	// used to update the item pulse effect
	mutable bool			inView;
	mutable int				inViewTime;
	mutable int				lastCycle;
	mutable int				lastRenderViewTime;

	// synced through snapshots to indicate show/hide or pickupSkin state
	// -1 on a client means undef, 0 not ready, 1 ready
public: // FIXME: Temp hack while Eric gets back to me about why GameState.cpp is trying to access this directly
	int						srvReady;
private: // FIXME: Temp hack while Eric gets back to me about why GameState.cpp is trying to access this directly
	int						clReady;

	int						itemPVSArea;

	bool					UpdateRenderEntity(renderEntity_s* renderEntity, const renderView_t* renderView) const;
	static bool				ModelCallback(renderEntity_s* renderEntity, const renderView_t* renderView);


	void					Event_Trigger(idEntity* activator);
	void					Event_DropToFloor();

	// RAVEN BEGIN
	// abahr
	void					Event_SetGravity();
	// RAVEN END
};

#endif // !__GAME_MACHINE_H__
