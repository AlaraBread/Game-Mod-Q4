#ifndef __GAME_MACHINE_H__
#define __GAME_MACHINE_H__

class Machine : public idEntity {
public:
	Machine();
	virtual					~Machine();

	void					Spawn(void);
	void					Think(void);

	// save games
	void					Save(idSaveGame* savefile) const;					// archives object for save game file
	void					Restore(idRestoreGame* savefile);					// unarchives object from save game file

	static const char* GetSpawnClassname(void);

	CLASS_PROTOTYPE(Machine);
};

#endif // !__GAME_MACHINE_H__
	