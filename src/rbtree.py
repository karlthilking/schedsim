from enum import Enum
from collections import deque

class rb_tree:
    class rb_color(Enum):
        red = 0
        black = 1

    class rb_direction(Enum):
        left = 0
        right = 1

    class rb_node:
        def __init__(self, val, color, left, right, parent):
            self.val = val
            self.color = color
            self.left = left
            self.right = right
            self.parent = parent
    
    def __init__(self):
        self.root = None
        self.min_vruntime = None

    def insert(self, val):
        if self.root == None:
            self.root = self.rb_node(
                val, self.rb_color.black, None, None, None
            )
            return
        node = self.root
        while True:
            if val > node.val:
                if node.right:
                    node = node.right
                else:
                    node.right = self.rb_node(
                        val, self.rb_color.red, None, None, None
                    )
                    return
            elif val <= node.val: 
                if node.left:
                    node = node.left
                else:
                    node.left = self.rb_node(
                        val, self.rb_color.red, None, None, None
                    )
                    return

    def rotate_left(self, x):
    #     
    #      p              p
    #     / \            / \
    #    x   *   ->     y   *
    #   / \            / \
    #  a   y          x   c
    #     / \        / \
    #    b   c      a   b
    #
        y = x.right
        x.right = y.left
        if y.left:
            y.left.parent = x
        y.parent = x.parent
        if x.parent is None:
            self.root = y
        elif x == x.parent.left:
            x.parent.left = y
        else:
            x.parent.right = y
        y.left = x
        x.parent = y

    def rotate_right(self, y):
    #
    #        p            p
    #       / \          / \
    #      y   *  ->    x   *
    #     / \          / \
    #    x   c        a   y
    #   / \              / \
    #  a   b            b   c
    #
        x = y.left
        y.left = x.right
        if x.right:
            x.right.parent = y
        x.parent = y.parent
        if y.parent is None:
            self.root = x
        elif y == y.parent.left:
            y.parent.left = x
        else:
            y.parent.right = x
        x.right = y
        y.parent = x


    def get_leftmost(self):
        node = self.root
        while True:
            if node.left:
                node = node.left
            elif node.right:
                node = node.right
            else:
                return node

    def display(self):
        q = deque()
        q.append(self.root)
        i = 1
        while len(q) > 0:
            cur = q.pop()
            left_val = -1
            right_val = -1
            if cur.left:
                q.append(cur.left)
                left_val = cur.left.val
            if cur.right:
                q.append(cur.right)
                right_val = cur.right.val
            print(f'{i}: {cur.val},{cur.color},{left_val},{right_val}')
            i += 1

