#include <stdio.h>

#include "../shared-obj.hpp"


struct Bar : public ppc::shared_obj_base {
	Bar() : id(next_id++) {
		printf("Bar[%d; %p]::Bar()\n", id, (void*)this);
	}

	Bar(const Bar &foo) : shared_obj_base(), id(next_id++) {
		printf("Bar[%d; %p]::Bar(const Bar &[%d; %p])\n", id, (void*)this,
		       foo.id, (void*)&foo);
	}

	~Bar() {
		printf("Bar[%d; %p]::~Bar()\n", id, (void*)this);
	}

private:
	int id;
	static int next_id;
};

int Bar::next_id = 0;



int main(void) {
	{
		puts("Shared Object");
		ppc::shared_obj<const Bar> ptr;
		printf("%p\n", (void*)ptr.get());
		ptr = new Bar();
		printf("%p\n", (void*)ptr.get());
		ptr = new Bar();
		printf("%p\n", (void*)ptr.get());
		ppc::shared_obj<const Bar> ptr2(new Bar());
		printf("%p %p\n", (void*)ptr.get(), (void*)ptr2.get());
		ptr2 = ptr;
		printf("%p %p\n", (void*)ptr.get(), (void*)ptr2.get());
	}
}
