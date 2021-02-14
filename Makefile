compileAll: ClockSynchBerkeley CausalorderedMulticast NoncausalMulticast

ClockSynchBerkeley: 
	g++ -o ClockSynchBerkeley ClockSynchBerkeley.cpp -lpthread

CausalorderedMulticast: 
	g++ -o CausalorderedMulticast CausalOrderedMulticast.cpp -lpthread

NoncausalMulticast:
	g++ -o NoncausalMulticast NonCausalMulticast.cpp -lpthread

BerkeleySlaveRun:
	./ClockSynchBerkeley process n

BerkeleyMasterRun:
	./ClockSynchBerkeley master

CausalRun:
	./CausalorderedMulticast 0 Message

NonCausalRun:
	./NoncausalMulticast 0 Message

clean:
	rm -rf *.o ClockSynchBerkeley CausalorderedMulticast NoncausalMulticast
