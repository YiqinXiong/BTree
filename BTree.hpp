#ifndef _BTREE_H
#define _BTREE_H
#define m 5 //设定B树的阶数

const int keyNumMax = m - 1;       //结点的最大关键字数量
const int keyNumMin = (m - 1) / 2; //结点的最小关键字数量（m/2取上界-1等同于(m-1)/2取下界）
typedef int KeyType;               //KeyType为关键字类型
typedef int Record;                //Record为记录类型

// 结点类型
// 所有非终端节点中包含(n,A0,K1,A1,K2,A2...,Kn,An)
// 其中n为关键字个数，Ki为关键字，Ai-1为小于Ki的节点指针，Ai为大于Ki的节点指针
typedef struct BTNode
{
    int keynum;                //结点关键字个数
    struct BTNode *parent;     //指向双亲结点
    KeyType key[m + 1];        //关键字数组，0号单元未用
    struct BTNode *ptr[m + 1]; //孩子结点指针数组
    Record recptr[m + 1];      //记录数组，0号单元未用
    BTNode() : keynum(), parent(), key(), ptr(), recptr() {}
} BTNode, *BTree;

// 查找结果类型
struct Result
{
    BTNode *pt; //指向找到的结点
    int i;      //在结点中的关键字位置;
    bool found; //查找成功与否标志
    Result() : pt(), i(), found() {}
    Result(BTNode *pt, int i, bool found) : pt(pt), i(i), found(found) {}
};

// 状态返回值枚举
typedef enum
{
    OK,
    ERROR,
    EMPTY
} Status;

// 初始化B树
Status InitBTree(BTree &t);

// 在结点p中查找关键字k的插入位置i
int SearchBTNode(BTNode *p, KeyType k);

/* 在树t上查找关键字k,返回结果(pt,i,tag)。
成功则tag=1,关键字k是指针pt所指结点中第i个关键字；
失败则tag=0,关键字k的插入位置为pt结点的第i个 */
Result SearchBTree(BTree t, KeyType k);

// 将关键字k和结点q分别插入到p->key[i+1]和p->ptr[i+1]中
void InsertBTNode(BTNode *&p, int i, KeyType k, BTNode *q);

// 将结点p分裂成两个结点,前一半保留,后一半移入结点q
KeyType SplitBTNode(BTNode *&p, BTNode *&q);

// 生成新的根结点t,原结点p和结点q为子树指针
void NewRoot(BTNode *&t, KeyType k, BTNode *p, BTNode *q);

/* 在树t上结点p的key[i]与key[i+1]之间插入关键字k。
若引起结点过大,则沿双亲链进行必要的结点分裂调整,使t仍是B树 */
Status InsertBTree(BTree &t, int i, KeyType k, BTNode *p);

// 从p结点删除key[i]和它的孩子指针ptr[i]
void Remove(BTNode *p, int i);

// 查找被删关键字p->key[i](在非叶子结点中)的替代叶子结点(右子树中值最小的关键字)
// 参数q：p->key[i]的右子树
KeyType FindReplace(BTNode *q);

/* 将双亲节点p中的最后一个关键字移入右节点q中，
将左节点aq中的最后一个关键字移入双亲节点p中 */
void MoveRight(BTNode *p, int i);

/* 将双亲节点p中的第一个关键字移入左节点aq中，
将右节点q中的第一个关键字移入双亲节点p中 */
void MoveLeft(BTNode *p, int i);

/* 将双亲结点p、右结点q合并入左结点aq，
并调整双亲结点p中的剩余关键字的位置 */
void Combine(BTNode *p, int i);

// 删除结点p中的第i个关键字后调整B树
void AdjustBTree(BTNode *p, int i);

// 在结点p中找到关键字k
bool FindBTNode(BTNode *p, KeyType k, int &i);

// 在结点p中查找并删除关键字k，若无则递归向下查找并删除
bool BTNodeDelete(BTNode *p, KeyType k);

// 删除操作
void BTreeDelete(BTree &t, KeyType k);

// 递归释放B树
void DestroyBTree(BTree &t);

// 层序遍历
void LevelTraverse(BTree t);

// 打印B树
Status PrintBTree(BTree t);

// 测试B树功能函数
void Test();
#endif