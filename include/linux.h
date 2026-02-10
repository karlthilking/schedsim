namespace Linux {
/* Approximation of RB-Tree used in the Linux Kernel for the Linux CFS */
class rb_tree {
/*                       root
 *                        13 
 *                      /    \
 *                    /        \
 *                   8         17
 *                 /  \        /  \
 * min_vruntime: 1     11    15    25
 *              / \    / \   /\    / \
 *             *   6  *   * *  * 22    27
 *                / \           /  \   /  \
 *               *   *         /    \ *    *
 *                            *      *
 */
private:
    enum rb_node_color {
        red     = 0,
        black   = 1
    };
    enum rb_node_direction {
        left    = 0,
        right   = 1
    };
    struct rb_node {
        rb_node_color   rb_color;
        rb_node         *rb_parent;
        rb_node         *rb_left;
        rb_node         *rb_right;
        
        rb_node_direction
        get_direction() const noexcept;
    };
    rb_node *root;
    rb_node *min_vruntime;
public:
    rb_tree() noexcept;
    
    rb_node *
    get_min_vruntime() const noexcept;

    void
    insert(rb_node *node) noexcept;

    void
    remove(rb_node *node) noexcept;

    void
    recolor() noexcept;

    void
    rearrange() noexcept;
};

/* Simulation/Approximation of the Linux Completely-Fair Scheduler */
class cfs {
private:
public:
};

/* Simulation/Approximation of the Linux O(1) Scheduler */
class O1 {
private:
public:
};

} // namespace Linux
