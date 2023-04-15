import matplotlib.pyplot as plt
import sys

def static1(option):
    if option[1] in [1,2,3,4,5,6,11,12]:
        filelist = ["pt","mem","tr"]
    elif option[1] in [7,8,9,10]:
        filelist = ["pt","nsat", "seu","tmr"]
    for file in filelist:
        with open(file, "r", encoding='utf-8') as f:
            time = []
            x = []
            for line in f:
                line = line.strip('\n\t').split(' ')
                x.append(line[0])
                if option[0] == 1:
                    time.append(float(line[1]))
                elif option[0] == 2:
                    time.append(float(line[1]))
            plt.plot(x, time, label=file)
    plt.xlabel("malloc count",fontdict={'size': 13,'color':  'k'})
    if option[1] in [11,12]:
        plt.ylabel("memory usage",fontdict={'size': 13,'color':  'k'})
    else:
        plt.ylabel("time",fontdict={'size': 13,'color':  'k'})
    if option[1] in [1,2]:
        plt.xlabel("size of block",fontdict={'size': 13,'color':  'k'})
    plt.tick_params(labelsize=12)
    ax = plt.gca()
    plt.tight_layout()
    ax.set_xticklabels(ax.get_xticklabels(), rotation=40, ha="right")
    plt.legend()
    if option[1] == 1:
        plt.title("malloc small block ",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp1.png',bbox_inches='tight',dpi=300)
    elif option[1] == 2:
        plt.title("malloc small block in 20 thread ",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp2.png',bbox_inches='tight',dpi=300)
    elif option[1] == 3:
        plt.title("malloc cost in 1 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp3.png',bbox_inches='tight',dpi=300)
    elif option[1] == 4:
        plt.title("malloc cost in 20 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp4.png',bbox_inches='tight',dpi=300)
    elif option[1] == 5:
        plt.title("malloc cost in 1 thread(random)",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp5.png',bbox_inches='tight',dpi=300)
    elif option[1] == 6:
        plt.title("malloc cost in 20 thread(random)",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp6.png',bbox_inches='tight',dpi=300)
    elif option[1] == 7:
        plt.title("malloc cost with or without SEU and TMR",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp7.png',bbox_inches='tight',dpi=300)
    elif option[1] == 8:
        plt.title("malloc cost with or without SEU and TMR in 20 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp8.png',bbox_inches='tight',dpi=300)
    elif option[1] == 9:
        plt.title("Read/Write cost",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp9.png',bbox_inches='tight',dpi=300)
    elif option[1] == 10:
        plt.title("Read/Write cost with or without SEU and TMR in 20 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp10.png',bbox_inches='tight',dpi=300)
    elif option[1] == 11:
        plt.title("memory cost in 1 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp11.png',bbox_inches='tight',dpi=300)
    elif option[1] == 12:
        plt.title("memory cost in 20 thread",fontdict={'size': 13,'color':  'k'})
        plt.savefig('./images/exp12.png',bbox_inches='tight',dpi=300)
    

if __name__=="__main__":
    if len(sys.argv) < 2:
        print('usage: python static_analy.py [eval num] [other options]')
        sys.exit(-1)
    else:
        eval_num = int(sys.argv[1])

    if eval_num == 1:
        static1((1, 1, 1))
    if eval_num == 2:
        static1((2, 2, 20))
    if eval_num == 3:
        static1((1, 3, 1))
    if eval_num == 4:
        static1((2, 4, 20))
    if eval_num == 5:
        static1((1, 5, 1))
    if eval_num == 6:
        static1((1, 6, 1))
    if eval_num == 7:
        static1((1, 7, 10))
    if eval_num == 8:
        static1((1, 8, 1))
    if eval_num == 9:
        static1((1, 9, 10))
    if eval_num == 10:
        static1((1, 10, 10))
    if eval_num == 11:
        static1((1, 11, 1))
    if eval_num == 12:
        static1((1, 12, 1))
