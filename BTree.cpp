#include "BTree.hpp"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <queue>

using namespace std;

// 初始化B树
Status InitBTree(BTree &t)
{
    t = nullptr;
    return OK;
}

// 在结点p中查找关键字k的位置i
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
成功则tag=1,关键字k是指针pt所指结点中第i个关键字；
失败则tag=0,关键字k的插入位置为pt结点的第i个 */
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
    {
        return Result(p, i, 1); // 查找成功，返回其在B树中位置
    }
    else
    {
        return Result(q, i, 0); // 查找不成功，返回其插入位置
    }
}

/*--------------------------------------------------------------------*/

// 将关键字k和结点q分别插入到p->key[i+1]和p->ptr[i+1]中
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

// 将插入新节点后的节点p（含有m个key，已经节点过大）分裂成两个结点，并返回分裂点x
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

// 生成新的根结点t,原结点p和结点q为子树指针
void NewRoot(BTree &t, KeyType k, BTNode *p, BTNode *q)
{
    t = new BTNode;
    t->keynum = 1;
    t->ptr[0] = p;
    t->ptr[1] = q;
    t->key[1] = k;
    // 调整结点p和结点q的双亲指针
    if (p != nullptr)
        p->parent = t;
    if (q != nullptr)
        q->parent = t;
    t->parent = nullptr;
}

/* 在树t上结点p的key[i]与key[i+1]之间插入关键字k。
若引起结点过大,则沿双亲链进行必要的结点分裂调整,使t仍是B树 */
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

/*--------------------------------------------------------------------*/
#ifdef DELETE
// 从p结点删除key[i]和它的孩子指针ptr[i]
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

// 查找被删关键字p->key[i](在非叶子结点中)的替代叶子结点(右子树中值最小的关键字)
void Substitution(BTNode *p, int i)
{
    //查找被删关键字p->key[i](在非叶子结点中)的替代叶子结点(右子树中值最小的关键字)
    BTNode *q;
    for (q = p->ptr[i]; q->ptr[0] != nullptr; q = q->ptr[0])
        ;
    p->key[i] = q->key[1]; //复制关键字值
}

/* 将双亲结点p中的最后一个关键字移入右结点q中，
将左结点aq中的最后一个关键字移入双亲结点p中 */
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

    q->ptr[1] = q->ptr[0]; //从双亲结点p移动关键字到右兄弟q中
    q->key[1] = p->key[i];
    q->keynum++;

    p->key[i] = aq->key[aq->keynum]; //将左兄弟aq中最后一个关键字移动到双亲结点p中
    p->ptr[i]->ptr[0] = aq->ptr[aq->keynum];
    aq->keynum--;
}

/* 将双亲结点p中的第一个关键字移入结点aq中，
将结点q中的第一个关键字移入双亲结点p中 */
void MoveLeft(BTNode *p, int i)
{
    int j;
    BTNode *aq = p->ptr[i - 1];
    BTNode *q = p->ptr[i];

    aq->keynum++; //把双亲结点p中的关键字移动到左兄弟aq中
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

/* 将双亲结点p、右结点q合并入左结点aq，
并调整双亲结点p中的剩余关键字的位置 */
void Combine(BTNode *p, int i)
{
    int j;
    BTNode *q = p->ptr[i];
    BTNode *aq = p->ptr[i - 1];

    aq->keynum++; //将双亲结点的关键字p->key[i]插入到左结点aq
    aq->key[aq->keynum] = p->key[i];
    aq->ptr[aq->keynum] = q->ptr[0];

    for (j = 1; j <= q->keynum; j++)
    { //将右结点q中的所有关键字插入到左结点aq
        aq->keynum++;
        aq->key[aq->keynum] = q->key[j];
        aq->ptr[aq->keynum] = q->ptr[j];
    }

    for (j = i; j < p->keynum; j++)
    { //将双亲结点p中的p->key[i]后的所有关键字向前移动一位
        p->key[j] = p->key[j + 1];
        p->ptr[j] = p->ptr[j + 1];
    }
    p->keynum--; //修改双亲结点p的keynum值
    delete q;    //释放空右结点q的空间
}

// 删除结点p中的第i个关键字后,调整B树
void AdjustBTree(BTNode *p, int i)
{
    if (i == 0)                            //删除的是最左边关键字
        if (p->ptr[1]->keynum > keyNumMin) //右结点可以借
            MoveLeft(p, 1);
        else //右兄弟不够借
            Combine(p, 1);
    else if (i == p->keynum)                   //删除的是最右边关键字
        if (p->ptr[i - 1]->keynum > keyNumMin) //左结点可以借
            MoveRight(p, i);
        else //左结点不够借
            Combine(p, i);
    else if (p->ptr[i - 1]->keynum > keyNumMin) //删除关键字在中部且左结点够借
        MoveRight(p, i);
    else if (p->ptr[i + 1]->keynum > keyNumMin) //删除关键字在中部且右结点够借
        MoveLeft(p, i + 1);
    else //删除关键字在中部且左右结点都不够借
        Combine(p, i);
}

// 反映是否在结点p中是否查找到关键字k
int FindBTNode(BTNode *p, KeyType k, int &i)
{
    if (k < p->key[1])
    { //结点p中查找关键字k失败
        i = 0;
        return 0;
    }
    else
    { //在p结点中查找
        i = p->keynum;
        while (k < p->key[i] && i > 1)
            i--;
        if (k == p->key[i]) //结点p中查找关键字k成功
            return 1;
    }
    return 0;
}

// 在结点p中查找并删除关键字k
int BTNodeDelete(BTNode *p, KeyType k)
{
    int i;
    int found_tag; //查找标志
    if (p == nullptr)
        return 0;
    else
    {
        found_tag = FindBTNode(p, k, i); //返回查找结果
        if (found_tag == 1)
        { //查找成功
            if (p->ptr[i - 1] != nullptr)
            {                                       //删除的是非叶子结点
                Substitution(p, i);                 //寻找相邻关键字(右子树中最小的关键字)
                BTNodeDelete(p->ptr[i], p->key[i]); //执行删除操作
            }
            else
                Remove(p, i); //从结点p中位置i处删除关键字
        }
        else
            found_tag = BTNodeDelete(p->ptr[i], k); //沿孩子结点递归查找并删除关键字k
        if (p->ptr[i] != nullptr)
            if (p->ptr[i]->keynum < keyNumMin) //删除后关键字个数小于MIN
                AdjustBTree(p, i);             //调整B树
        return found_tag;
    }
}

// 构建删除框架，执行删除操作
void BTreeDelete(BTree &t, KeyType k)
{
    BTNode *p;
    int a = BTNodeDelete(t, k); //删除关键字k
    if (a == 0)                 //查找失败
        printf("key[%d] is not exist!\n", k);
    else if (t->keynum == 0)
    { //调整
        p = t;
        t = t->ptr[0];
        delete p;
    }
}

// 递归释放B树
void DestroyBTree(BTree &t)
{
    int i;
    BTNode *p = t;
    if (p != nullptr)
    { //B树不为空
        for (i = 0; i <= p->keynum; i++)
        { //递归释放每一个结点
            DestroyBTree(*&p->ptr[i]);
        }
        delete p;
    }
    t = nullptr;
}
#endif
/*--------------------------------------------------------------------*/

// 用队列遍历输出B树
void LevelTraverse(BTree t)
{
    queue<BTNode *> que;
    BTNode *p;
    int length;

    que.push(t);
    while (!que.empty())
    {
        length = que.size(); // 获取当前队列长度，用于分层
        //打印当前层所有节点
        for (int i = 0; i < length; i++)
        {
            // 弹出头节点作为当前节点
            p = que.front();
            que.pop();
            // 打印当前节点
            printf("|");
            for (int j = 1; j <= p->keynum; j++)
            {
                printf("%d\t", p->key[j]);
            }
            printf("|\t");
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

// 输出B树
Status PrintBTree(BTree t)
{
    if (t == NULL)
    {
        printf("Empty B-Tree!\n");
        return EMPTY;
    }
    LevelTraverse(t);
    return OK;
}

/*--------------------------------------------------------------------*/

// 测试B树功能函数
void Test()
{
    BTNode *t = nullptr;
    Result s; //设定查找结果
    int j, n = 15;
    KeyType k;
    KeyType a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    printf("Create B-Tree(m=%d):\n", m);
    for (j = 0; j < n; j++)
    { //逐一插入元素
        s = SearchBTree(t, a[j]);
        if (!s.found)
            InsertBTree(t, s.i, a[j], s.pt);
        printf("step %d, insert elem[%d]:\n ", j + 1, a[j]);
        PrintBTree(t);
    }

    printf("\n");
}

#ifdef TEST
void Test1()
{
    system("color 70");
    BTNode *t = NULL;
    Result s; //设定查找结果
    int j, n = 15;
    KeyType k;
    KeyType a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    printf("创建一棵%d阶B树:\n", m);
    for (j = 0; j < n; j++)
    { //逐一插入元素
        s = SearchBTree(t, a[j]);
        if (s.tag == 0)
            InsertBTree(t, s.i, a[j], s.pt);
        printf("   第%d步,插入元素%d:\n ", j + 1, a[j]);
        PrintBTree(t);
    }

    printf("\n");
    printf("删除操作:\n"); //删除操作
    k = 9;
    BTreeDelete(t, k);
    printf("  删除%d:\n ", k);
    printf("  删除后的B树: \n");
    PrintBTree(t);
    printf("\n");

    k = 1;
    BTreeDelete(t, k);
    printf("  删除%d:\n ", k);
    printf("  删除后的B树: \n");
    PrintBTree(t);
    printf("\n");

    printf("  递归释放B树\n"); //递归释放B树
    DestroyBTree(t);
    PrintBTree(t);
}

void Test2()
{
    int i, k;
    system("color 70");
    BTree t = NULL;
    Result s; //设定查找结果
    while (1)
    {
        printf("此时的B树：\n");
        PrintBTree(t);
        printf("\n");
        printf("=============Operation Table=============\n");
        printf("   1.Init     2.Insert    3.Delete    \n");
        printf("   4.Destroy  5.Exit      \n");
        printf("=========================================\n");
        printf("Enter number to choose operation:_____\b\b\b");
        scanf("%d", &i);
        switch (i)
        {
        case 1:
        {
            InitBTree(t);
            printf("InitBTree successfully.\n");
            break;
        }

        case 2:
        {
            printf("Enter number to InsertBTree:_____\b\b\b");
            scanf("%d", &k);
            s = SearchBTree(t, k);
            InsertBTree(t, s.i, k, s.pt);
            printf("InsertBTree successfully.\n");
            break;
        }
        case 3:
        {
            printf("Enter number to DeleteBTree:_____\b\b\b");
            scanf("%d", &k);
            BTreeDelete(t, k);
            printf("\n");
            printf("DeleteBTree successfully.\n");
            break;
        }
        case 4:
        {
            DestroyBTree(t);
            break;
            printf("DestroyBTree successfully.\n");
        }
        case 5:
        {
            exit(-1);
            break;
        }
        }
    }
}
#endif

int main()
{
    Test();
    return 0;
}
