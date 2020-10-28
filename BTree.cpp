#include <queue>
#include <iostream>
#include "BTree.hpp"

using namespace std;

#define SEARCH
#define INSERT
#define DELETE
#define OUTPUT
#define TEST
/*--------------------------------------------------------------------*/
#ifdef SEARCH
// 初始化B树
Status InitBTree(BTree &t)
{
    t = nullptr;
    return OK;
}

// 在节点p中查找关键字k的位置i
// i使得：p->key[i] <= k < p->key[i+1]
// 找到对应key则i就是key的下标，没找到对应key则i就是继续向下查找的指针的下标
int SearchBTNode(BTNode *p, KeyType k)
{
    int i = 0;
    while (i < p->keynum && k >= p->key[i + 1])
    {
        i++;
    }
    return i;
}

/* 在树t上查找关键字k,返回结果(pt,i,tag)。
成功则tag=1,关键字k是指针pt所指节点中第i个关键字；
失败则tag=0,关键字k的插入位置为pt节点的第i个 */
Result SearchBTree(BTree t, KeyType k)
{
    // 初始化，p指向待查节点，q指向p的双亲
    BTNode *p = t;
    BTNode *q = nullptr;
    bool found = false;
    int i = 0;
    //while循环查找过程，条件1：p非空（没到叶子节点），条件2：还没找到
    while (p && !found)
    {
        i = SearchBTNode(p, k);
        if (i > 0 && p->key[i] == k)
        {
            found = true;
        }
        else
        {
            q = p;
            p = p->ptr[i];
            found = false;
        }
    }
    if (found)
        return Result(p, i, 1); // 查找成功，返回其在B树中位置
    else
        return Result(q, i, 0); // 查找不成功，返回其插入位置
}
#endif
/*--------------------------------------------------------------------*/
#ifdef INSERT
// 将关键字k和节点q分别插入到p->key[i+1]和p->ptr[i+1]中
void InsertBTNode(BTNode *&p, int i, KeyType k, BTNode *q)
{
    // i+1之后（包括原i+1）整体后移，空出位置i+1
    int j;
    for (j = p->keynum; j > i; j--)
    {
        p->key[j + 1] = p->key[j];
        p->ptr[j + 1] = p->ptr[j];
    }
    // 插入k和q
    p->key[i + 1] = k;
    p->ptr[i + 1] = q;
    if (q != nullptr)
        q->parent = p;
    p->keynum++;
}

// 将插入新节点后的节点p（含有m个key，已经节点过大）分裂成两个节点，并返回分裂点x
KeyType SplitBTNode(BTNode *&p, BTNode *&q)
{
    int s = (m + 1) / 2;
    q = new BTNode;        //新建节点q
    q->ptr[0] = p->ptr[s]; //后一半移入节点q
    for (int i = s + 1; i <= m; i++)
    {
        q->key[i - s] = p->key[i];
        q->ptr[i - s] = p->ptr[i];
        q->recptr[i - s] = p->recptr[i];
    }
    q->keynum = p->keynum - s; //节点q存放m-s个
    q->parent = p->parent;
    //转移到节点q之后，修改双亲指针
    for (int i = 0; i <= q->keynum; i++)
    {
        if (q->ptr[i] != nullptr)
            q->ptr[i]->parent = q;
    }
    p->keynum = s - 1; //节点p存放s个，但保留s-1个，p中最后一个key作为分裂点x
    return p->key[s];
}

// 生成新的根节点t,原节点p和节点q为子树指针
void NewRoot(BTree &t, KeyType k, BTNode *p, BTNode *q)
{
    t = new BTNode;
    t->keynum = 1;
    t->ptr[0] = p;
    t->ptr[1] = q;
    t->key[1] = k;
    // 调整节点p和节点q的双亲指针
    if (p != nullptr)
        p->parent = t;
    if (q != nullptr)
        q->parent = t;
    t->parent = nullptr;
}

/* 在树t上节点p的key[i]与key[i+1]之间插入关键字k。
若引起节点过大,则沿双亲链进行必要的节点分裂调整,使t仍是B树 */
Status InsertBTree(BTree &t, int i, KeyType k, BTNode *p)
{
    bool finished = false;
    BTNode *q = nullptr;
    KeyType x = k;
    while (p && !finished)
    {
        InsertBTNode(p, i, x, q);
        if (p->keynum < m)
            finished = true;
        else
        {
            x = SplitBTNode(p, q);
            //若p的双亲存在，在双亲节点中找x的插入位置
            p = p->parent;
            if (p)
            {
                i = SearchBTNode(p, x);
            }
        }
    }
    //未完成，情况1：t是空树（初始p为nullptr），情况2：根节点分裂为p和q
    if (!finished)
    {
        p = t; //原t即为p
        NewRoot(t, x, p, q);
    }
    return OK;
}
#endif
/*--------------------------------------------------------------------*/
#ifdef DELETE
// 从p节点删除key[i]和它的孩子指针ptr[i]
void Remove(BTNode *p, int i)
{
    // 从i+1位置开始向前移动覆盖
    int j;
    for (j = i + 1; j <= p->keynum; j++)
    {
        p->key[j - 1] = p->key[j];
        p->ptr[j - 1] = p->ptr[j];
    }
    p->keynum--;
}

// 查找被删关键字p->key[i](在非叶子节点中)的替代叶子节点(右子树中值最小的关键字)
// 参数q：p->key[i]的右子树
KeyType FindReplace(BTNode *q)
{
    while (q->ptr[0] != nullptr)
    {
        q = q->ptr[0];
    }
    return q->key[1]; //右子树中最小的关键字
}

/* 将双亲节点p中的最后一个关键字移入右节点q中，
将左节点aq中的最后一个关键字移入双亲节点p中 */
void MoveRight(BTNode *p, int i)
{
    int j;
    BTNode *q = p->ptr[i];
    BTNode *aq = p->ptr[i - 1];

    for (j = q->keynum; j > 0; j--)
    { //将右兄弟q中所有关键字向后移动一位
        q->key[j + 1] = q->key[j];
        q->ptr[j + 1] = q->ptr[j];
    }

    q->ptr[1] = q->ptr[0]; //从双亲节点p移动关键字到右兄弟q中
    q->key[1] = p->key[i];
    q->keynum++;

    p->key[i] = aq->key[aq->keynum]; //将左兄弟aq中最后一个关键字移动到双亲节点p中
    p->ptr[i]->ptr[0] = aq->ptr[aq->keynum];
    aq->keynum--;
}

/* 将双亲节点p中的第一个关键字移入左节点aq中，
将右节点q中的第一个关键字移入双亲节点p中 */
void MoveLeft(BTNode *p, int i)
{
    int j;
    BTNode *aq = p->ptr[i - 1];
    BTNode *q = p->ptr[i];

    aq->keynum++; //把双亲节点p中的关键字移动到左兄弟aq中
    aq->key[aq->keynum] = p->key[i];
    aq->ptr[aq->keynum] = p->ptr[i]->ptr[0];

    p->key[i] = q->key[1]; //把右兄弟q中的关键字移动到双亲节点p中
    q->ptr[0] = q->ptr[1];
    q->keynum--;

    for (j = 1; j <= aq->keynum; j++)
    { //将右兄弟q中所有关键字向前移动一位
        aq->key[j] = aq->key[j + 1];
        aq->ptr[j] = aq->ptr[j + 1];
    }
}

/* 将双亲节点p、右节点q合并入左节点aq，
并调整双亲节点p中的剩余关键字的位置 */
void Combine(BTNode *p, int i)
{
    int j;
    BTNode *q = p->ptr[i];
    BTNode *aq = p->ptr[i - 1];

    //将双亲节点的关键字p->key[i]插入到左节点aq
    aq->keynum++;
    aq->key[aq->keynum] = p->key[i];
    aq->ptr[aq->keynum] = q->ptr[0];

    //将右节点q中的所有关键字插入到左节点aq
    for (j = 1; j <= q->keynum; j++)
    {
        aq->keynum++;
        aq->key[aq->keynum] = q->key[j];
        aq->ptr[aq->keynum] = q->ptr[j];
    }

    for (j = i; j < p->keynum; j++)
    { //将双亲节点p中的p->key[i]后的所有关键字向前移动一位
        p->key[j] = p->key[j + 1];
        p->ptr[j] = p->ptr[j + 1];
    }
    p->keynum--; //修改双亲节点p的keynum值
    delete q;    //释放空右节点q的空间
}

// 删除节点p中的第i个关键字后，调整B树
void AdjustBTree(BTNode *p, int i)
{
    // 删除的是最左孩子节点中的关键字
    if (i == 0)
        if (p->ptr[1]->keynum > keyNumMin) //右节点够借
            MoveLeft(p, 1);
        else //右节点不够借
            Combine(p, 1);
    // 删除的是最右孩子节点中的关键字
    else if (i == p->keynum)
        if (p->ptr[i - 1]->keynum > keyNumMin) //左节点够借
            MoveRight(p, p->keynum);
        else //左节点不够借
            Combine(p, p->keynum);
    // 删除的是中间孩子节点中的关键字且左节点够借
    else if (p->ptr[i - 1]->keynum > keyNumMin)
        MoveRight(p, i);
    // 删除的是中间孩子节点中的关键字且右节点够借
    else if (p->ptr[i + 1]->keynum > keyNumMin)
        MoveLeft(p, i + 1);
    // 删除的是中间孩子节点中的关键字且左右都不够借
    else
        Combine(p, i);
}

// 反映是否在节点p中是否查找到关键字k
bool FindBTNode(BTNode *p, KeyType k, int &i)
{
    if (k < p->key[1])
    { //节点p中查找关键字k失败
        i = 0;
        return false;
    }
    else
    { //在p节点中查找
        i = p->keynum;
        while (k < p->key[i] && i > 1)
            i--;
        if (k == p->key[i]) //节点p中查找关键字k成功
            return true;
    }
    return false;
}

// 在节点p中查找并删除关键字k
bool BTNodeDelete(BTNode *p, KeyType k)
{
    int i;
    bool found; //查找标志
    if (p == nullptr)
        return 0;
    else
    {
        found = FindBTNode(p, k, i); //返回查找结果
        //查找成功
        if (found)
        {
            //删除的是非叶子节点的关键字
            //理解i-1：若为非叶子节点，被删除关键字为key[i]，则ptr[i-1]一定存在
            if (p->ptr[i - 1] != nullptr)
            {
                p->key[i] = FindReplace(p->ptr[i]); //寻找相邻关键字(右子树中最小的关键字)
                BTNodeDelete(p->ptr[i], p->key[i]); //执行删除操作
            }
            else
                Remove(p, i); //从节点p中位置i处删除关键字
        }
        else
            found = BTNodeDelete(p->ptr[i], k); //沿孩子节点递归查找并删除关键字k
        // 非叶子节点删除后可能从右子树替补，可能会使右子树关键字个数不足
        if (p->ptr[i] != nullptr)
            if (p->ptr[i]->keynum < keyNumMin) //删除后关键字个数不足
                AdjustBTree(p, i);             //调整B树
        return found;
    }
}

// 删除操作
void BTreeDelete(BTree &t, KeyType k)
{
    bool deleted = BTNodeDelete(t, k); //删除关键字k
    //查找失败
    if (!deleted)
        printf("key[%d] is not exist!\n", k);
    //删除后根节点无key
    else if (t->keynum == 0)
    {
        BTNode *p = t;
        t = t->ptr[0];
        delete p;
    }
}

// 递归释放B树
void DestroyBTree(BTree &t)
{
    if (!t)
        return;
    BTNode *p = t;
    for (int i = 0; i <= p->keynum; i++)
        DestroyBTree(p->ptr[i]);
    delete p;
    t = nullptr;
}
#endif
/*--------------------------------------------------------------------*/
#ifdef OUTPUT
// 层序遍历
void LevelTraverse(BTree t)
{
    queue<BTNode *> que;
    BTNode *p;
    int length;    //队列长度，用于控制每一层的节点个数
    int level = 0; //记录层数

    que.push(t);
    while (!que.empty())
    {
        length = que.size(); // 获取当前队列长度，用于分层
        //打印当前层所有节点
        printf("\tLevel %-2d:", level++);
        for (int i = 0; i < length; i++)
        {
            // 弹出头节点作为当前节点
            p = que.front();
            que.pop();
            // 打印当前节点
            printf("[");
            printf("%d", p->key[1]);
            for (int j = 2; j <= p->keynum; j++)
            {
                printf(", %d", p->key[j]);
            }
            printf("]  ");
            // 把当前节点的所有非空子节点加入队列
            for (int j = 0; j <= p->keynum; j++)
            {
                if (p->ptr[j] && p->ptr[j]->keynum != 0)
                    que.push(p->ptr[j]);
            }
        }
        printf("\n");
    }
}

// 打印B树
Status PrintBTree(BTree t)
{
    if (t == NULL)
    {
        printf("\tEmpty B-Tree!\n");
        return EMPTY;
    }
    LevelTraverse(t);
    return OK;
}
#endif
/*--------------------------------------------------------------------*/
#ifdef TEST
// 测试
void Test()
{
    BTree t;
    Result s; //设定查找结果
    KeyType k;
    const int n = 16;
    KeyType arr[n];
    for (int i = 0; i < n; i++)
        arr[i] = i + 1;
    printf("Create B-Tree(m=%d):\n", m);
    InitBTree(t);
    PrintBTree(t);
    //逐一插入元素
    for (int i = 0; i < n; i++)
    {
        s = SearchBTree(t, arr[i]);
        if (!s.found)
            InsertBTree(t, s.i, arr[i], s.pt);
        printf("step %d, insert elem[%d]:\n", i + 1, arr[i]);
        PrintBTree(t);
    }

    int op;
    while (1)
    {
        printf("\n---PrintBTree---\n");
        PrintBTree(t);
        printf("\n");
        printf("-----------------------------------------------------\n");
        printf("\t1.Init\t2.Insert\t3.Delete\t4.Destroy\t5.Exit\n");
        printf("-----------------------------------------------------\n");
        printf("Your option:");
        cin >> op;
        switch (op)
        {
        case 1:
            DestroyBTree(t);
            InitBTree(t);
            printf("InitBTree successfully.\n");
            break;
        case 2:
            printf("Enter number to InsertBTree:");
            scanf("%d", &k);
            s = SearchBTree(t, k);
            InsertBTree(t, s.i, k, s.pt);
            printf("InsertBTree successfully.\n");
            break;
        case 3:
            printf("Enter number to DeleteBTree:");
            scanf("%d", &k);
            BTreeDelete(t, k);
            printf("\n");
            printf("DeleteBTree successfully.\n");
            break;
        case 4:
            DestroyBTree(t);
            printf("DestroyBTree successfully.\n");
            break;
        case 5:
            DestroyBTree(t);
            exit(-1);
            break;
        }
    }
    printf("\n");
}
#endif

int main()
{
#ifdef TEST
    Test();
#endif
    return 0;
}
