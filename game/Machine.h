#ifndef __GAME_MACHINE_H__
#define __GAME_MACHINE_H__

class Machine : public idEntity {
public:
	CLASS_PROTOTYPE(Machine);

	Machine(void);

	void					Spawn(void);
	void					Think(void);

	void					Save(idSaveGame* savefile) const;
	void					Restore(idRestoreGame* savefile);

	// the amount of each type of item stored inside the machine
	int red;
	int green;
	int blue;
	int yellow;
	int magenta;
	int cyan;

private:

	idVec3					moveDir;
	float					moveSpeed;

	void					Event_FindTargets(void);

public:
	bool extractor;
	bool crafter;
	int timer;
};


#endif // !__GAME_MACHINE_H__
