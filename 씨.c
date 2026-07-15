static 원시_스핀락_선언(scx_sched_lock); 
/* 
어떤 스레드가 자물쇠를 얻으려하면 잠겨있을시에 sleep이 아닌 열릴때까지 루프를 돌며 대기하는 락 방식
즉 sleep에서 깨어나는 시간도 아까운 경우에 이용
*/

struct scx_sched __rcu *scx_root; /* __가 있으면 내부 검증이 생략되었거나 특정 조건에서만 작동하도록 설계된 함수라는 의미*/
/*
struct scx_sched 로 스케줄러 인스턴스를 나타낸다.
sched_ext의 경우 단일 스케줄러로 존재하는것이 아닌 cgroup 계층 구조에 따라 여러 서브 스케줄러가 중첩되어 실행되는 계층형 스케줄러 기능을 지원하기 때문에 이 각각의 스케줄러 상태를 담는 구조체가 바로 이것이다.

__rcu의 경우 앞서 적어뒀듯이 작업이 빈번한 경우 lock없이 빠르게 읽어주는 동기화 메커니즘이다.
__rcu가 앞에 붙은 포인터 like 여기서의 *scx_root같은 케이스에선 바로 *scx_root라고 적는다든가 scx_root->member 와 같은 방법으로 직접적인 역참조를 해서는 안된다. 반드시 rcu_dereference(scx_root)같은 전용 api를 거쳐야만 한다.
api는 다른말로 내장 함수라고 할 수 있겠는데, 이 내장함수를 이용하지 않고 struct scx_sched *root = scx_root;  와 같이 이용해버리면 컴파일러의 최적화라든가 cpu의 out of order execution이라든가 작동할시에 데이터 타이밍이 꼬여서 시스템이 망가지게 될것이다.
따라서 struct scx_sched *root = rcu_dereference(scx_root); 와 같이 내장함수를 이용하여 내장함수 규격에 맞추는것이 중요하다.

정리하자면 RCU 방식으로 동기화 시킨채로만 접근할 수 있는 스케줄러 인스턴스의 주소를 갖는 포인터 변수를 선언한것이다.
*/

static LIST_HEAD(scx_sched_all); 
/* 
연결리스트의 헤드를 만든다. 그리고 연결리스트는 각각 스케줄러 구조체들을 하나로 묶어서 관리할 목적이다. 
따라서 나머지 스케줄러 구조체들은 scx_sched_all 뒤로 차례차례 기차 칸 연결되듯이 붙어져 만들어진다.
*/
#ifdef/* 커널 옵션이 켜져있을때만 작동하되, 그 커널 옵션은~ */ cgroupq별로_하위_스케줄러를_독립적으로_실행및관리하는_기능_활성화
static const struct rhashtable_params scx_sched_hash_params = {
	.열쇠 크기		= sizeof_field(struct scx_sched, ops.sub_cgroup_id),
	.열쇠 위치		= offsetof(struct scx_sched, ops.sub_cgroup_id),
	.데이터 연결을 위한 구조체 내부 연결고리 변수 위치		= offsetof(struct scx_sched, hash_node),
	.해시 테이블이 커지면서 발생할법한 경고나 제한을 완화할것인가?	= true,	/* 어차피 넣고 뺄때마다 scx_sched_lock으로 lock해두고서 작업할거니까 빡빡하게 안하는게 성능도 잃지 않는 방향이라는게 이유. */
};

static struct rhashtable scx_sched_hash; /* r이 붙은 hashtable의 의미는 r이 "크기 조절 가능한", "상대적인" 이라는 의미라는것을 감안하여 이해하자. 다시말해 여러 cpu가 동시에 해시 테이블을 바라볼때에 한쪽 cpu가 테이블 크기를 늘린다든가 다른 어떤짓을 하더라도 신경쓰지 않고 각자의 관점에서 데이터를 안전하게 읽을 수 있다는 의미에서 r이 붙어졌다.*/
#endif/* ifdef는 "만약 이 옵션이 켜져있다면 아래 코드를 컴파일해라". endif는 "ifdef로 시작한 조건 체크는 여기서 끝내고, 이 아래부터는 옵션 상관없이 전부 컴파일해라" */

static const struct rhashtable_params scx_tid_hash_params/* 의미는 : 찾고자하는_프로세스의_스케줄러_정보를_tid로_찾아내기_위한_테이블_params */ = {
	.열쇠 크기		= sizeof_field(struct sched_ext_entity, tid),
	.열쇠 위치		= offsetof(struct sched_ext_entity, tid),
	.데이터 연결을 위한 구조체 내부 연결고리 변수 위치		= offsetof(struct sched_ext_entity, tid_hash_node),
	.해시 테이블이 커지면서 발생할만한 경고나 제한 완화 여부	= true 
};

/*
프로세스가 종료되는건 task가 exit된다고 표현합니다. 그런 과정에서 자신의 pid를 반납하여 결과적으로 본인 pid를 잃은 상황이라 할지라도 cpu에 의해 스케줄링 되어 실행되는것이 가능합니다.
이유는 다음과 같습니다.
1. 보통 프로그램이 종료가 된다고 해서 바로 메모리에서 사라지는건 아닙니다. 커널 내부에서 프로세스가 죽으면 단계적인 정리과정을 거칩니다.
2. 정리하는 중간에 "난 이제 죽었다" 하고 pid를 커널에 반납하는 단계가 있지 않겠습니까? 하지만 위에서 적어뒀듯이 그런다고해서 완전한 종료는 아닙니다.
3. pid의 반납후에 이뤄지는 free가 있어야 완전한 프로세스 킬인데요, 이 사이 찰나의 순간에도 cpu를 갖고 무언가를 실행하는 스케줄이 가능합니다.

신기하긴한데, 그래서 그게 뭐 어쨌다는거냐? 싶습니다.
만약 bpf 스케줄러를 끄고 원래의 기본 스케줄러로 되돌린다면 시스템의 안전을 위해 모든 프로세스를 전부 순회하며 찾아내야합니다.
이때 단 하나의 프로세스라도 미처 찾아내지 못해버린다면 그 프로세스는 평생 cpu를 할당받지 못한채로 고아가 됩니다. 
여기서 고려해야 하는게 아까 말한 임종 직전의 프로세스 입니다. 아무리 free 직전이래도 일하겠다고 노구를 이끌고 스케줄을 기다리는 중인데 그대로 유기해버려서야 도리가 아니겠죠?

그러면 어떻게 고아 프로세스 없는 세상을 만들수 있을까요?
답은 프로세스들이 출산되는 그 시점(fork를 의미합니다) 부터 메모리에서 사라지는 free 순간까지 전부 담아두는 리스트를 만드는것 입니다.

*/
