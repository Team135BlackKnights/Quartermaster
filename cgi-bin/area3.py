#!/usr/bin/env python

import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as NP
from argparse import ArgumentParser
from matplotlib import dates
from datetime import date

def print_lines(x):
    for elem in x:
        print(elem)


def plot(output,title,xlabel,ylabel,y,x,labels):
    #output file interface->string->str->str->[[num]]->[int]->[str]->void

    y_stack = NP.cumsum(y, axis=0)   # a 3x10 array

    fig = plt.figure()
    ax1 = fig.add_subplot(111)

    #Not sure why matplot lib is unable to automatically choose contrasting colors for this case, but whatever.
    colors=[
        '#ff0000','#00ff00','#0000ff',
        '#ffcccc','#ccffcc','#ccccff',
        '#000000','#ffffff','#cccccc',
        '#ffff00','#ff00ff','#00ffff'
        ]

    ax1.xaxis_date()
    ax1.xaxis.set_major_formatter(dates.DateFormatter('%b %d %Y'))

    for i in range(len(y_stack)):
        if i==0:
            start=0
        else:
            start=y_stack[i-1,:]
        ax1.fill_between(x, start, y_stack[i,:], facecolor=colors[i],label=labels[i])
    for tick in ax1.get_xticklabels():
        tick.set_rotation(-90)

    plt.title(title)
    plt.xlabel('Date')
    plt.ylabel('Number of parts')
    plt.legend(loc='upper left')
    plt.tight_layout() #avoid cutting off the x-axis labels
    plt.show()
    plt.savefig(output)


def get_stdin():
    r=[]
    try:
        while True:
            r.append(raw_input())
    except EOFError:
        return r


def parse_date(s):
    date1,time=s.split()
    sp=date1.split('-')
    assert len(sp)==3
    return date(int(sp[0]),int(sp[1]),int(sp[2]))


def parse_din(lines):
    #[str]->([str],[[float]])

    #print 'lines:'
    #print_lines(lines)

    title=lines[0]
    lines=lines[1:]

    xlabel=lines[0]
    lines=lines[1:]

    ylabel=lines[0]
    lines=lines[1:]

    labels=lines[0].split(',')[1:-1]

    def parse_line(s):
        sp=s.split(',')[:-1]
        #print 'spp:',sp
        return [parse_date(sp[0])]+map(float,sp[1:-1])
    items=map(parse_line,lines[1:])
    return title,xlabel,ylabel,labels,items


def main():
    p=ArgumentParser()
    p.add_argument('-o','--out')
    p.add_argument('--input',action='store_true')
    args=p.parse_args()
    if args.out:
        output=open(args.out,'w')
    else:
        output=sys.stdout

    if args.input:
        f=get_stdin()
        title,xlabel,ylabel,labels,data=parse_din(f)
        #print 'labels:',labels
        #print data
        def get_col(i): return map(lambda x: x[i],data)
        x=get_col(0)
        #print 'x:',x
        y=map(get_col,range(1,len(data[0])))
    else:
        # just create some random data
        fnx = lambda : NP.random.randint(3, 10, 10)
        y = NP.row_stack((fnx(), fnx(), fnx()))   
        x = NP.array([date.toordinal(date(2019,3,x)) for x in range(2,12)])
        labels='abc'
        title='Demo title'
        xlabel='xlabel'
        ylabel='ylabel'
    plot(output,title,xlabel,ylabel,y,x,labels=labels)


if __name__=='__main__':
    main()

