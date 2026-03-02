#include <semaphore.h>
#include <cstdlib>

class S {
private:
	sem_t *sems;
	int nsems;
public:
	S(int count) : nsems(count)
	{
		sems = (sem_t *)malloc(sizeof(sem_t) * nsems);
		for (int i = 0; i < nsems; ++i)
			sem_init(sems + i, 0, 0);
	}

	~S()
	{
		for (int i = 0; i < nsems; ++i)
			sem_destroy(sems + i);
	}
};

int
main(void)
{
	S s(5);
	_Exit(0);
}
